
#include <string.h>
#include <stdio.h>
#include <api_os.h>
#include <api_gps.h>
#include <api_event.h>
#include <api_hal_uart.h>
#include <api_debug.h>
#include "buffer.h"
#include "gps_parse.h"
#include "math.h"
#include "gps.h"
#include "api_hal_pm.h"
#include "time.h"
#include "api_info.h"
#include "assert.h"
#include "api_socket.h"
#include "api_network.h"
#include "api_hal_gpio.h"
#include "api_hal_watchdog.h"
#include "api_call.h"
#include "api_sms.h"
#include "api_hal_i2c.h"

/**
 * gps tracker, use an open source tracker server traccar:https://www.traccar.org/
 * the server in the code(`#define SERVER_IP   "ss.neucrack.com"`) may invalid someday, you can download the server and deploy youself
 * How to use: 
 *          compile and download to A9G dev board, open browser, access http://ss.neucrack.com:8082 ,
 *          then register and login, add devices and the number is IMEI e.g. `867959033006999`, finally the position of your device will be show in the map
 *          
 * @attention The code below just a demo, please read and check the code carefully before copy to your project directly(DO NOT copy directly)!!!!!!
 * 
 * 
 */
#define SERVER_IP   "demo3.traccar.org"
#define SERVER_PORT  5055

#define GPS_NMEA_LOG_FILE_PATH "/t/gps_nmea.log"



#define MAIN_TASK_STACK_SIZE    (2048 * 2)
#define MAIN_TASK_PRIORITY      0
#define MAIN_TASK_NAME          "GPS Test Task"

static HANDLE gpsTaskHandle = NULL;


bool isGpsOn = true;
bool networkFlag = false;
volatile bool falseGPS = true;
volatile double prev_lat = 0.0;
volatile double prev_long = 0.0;
volatile bool motion = true;   //Taking initial motion true, this logic is needed for Interrupt based restart fucntion
volatile bool restart_flag = false;
volatile bool init_done_flag = false;
volatile bool is_Fixed_flag = false;
volatile bool low_pow_mode_flag = false;
volatile uint8_t led_status;
//volatile bool close_interrupt = false;
volatile bool int_status = false;


#define SYSTEM_STATUS_LED       GPIO_PIN27
#define UPLOAD_DATA_LED         GPIO_PIN28
#define VIBRATION_SENS_INPUT    GPIO_PIN2
#define I2C_ACC                 I2C2

//User Defines
#define KEY_POWER               0x4B
#define GPS_MUL_FACTOR_MAX      1000
#define GPS_MUL_FACTOR_MIN      1000000
#define LAT_THRESHOLD_MAX       10          //Used to root out false readings, Based on formula (Difference between Extremes lat x MUL FACTOR)
#define LONG_THRESHOLD_MAX      10

#define LAT_THRESHOLD_MIN       4     //Ne
#define LONG_THRESHOLD_MIN      4

#define TWO_D_FIX               "2D Fix"
#define THREE_D_FIX             "3D Fix"
#define DIFF_GPS_FIX            "3D/DGPS fix"
#define NO_FIX                  "no fix"

//I2C Defines
#define MPU_ADDRESS             0x68
#define SIGNAL_PATH_RESET       0x68
#define I2C_SLV0_ADDR           0x37
#define ACCEL_CONFIG            0x1C
#define MOT_THR                 0x1F // Motion detection threshold bits [7:0]
#define MOT_DUR                 0x20 // Duration counter threshold for motion interrupt generation, 1 kHz rate, LSB = 1 ms
#define MOT_DETECT_CTRL         0x69
#define INT_ENABLE              0x38
#define WHO_AM_I_MPU6050        0x75 // Should return 0x68
#define INT_STATUS              0x3A

//Custumisation
#define USE_WATCHDOG            1
#define USE_TRACE               1
#define USE_UART                1
#define USE_VIBRATION_SENSOR    0
#define USE_MPU_SENSOR          1
#define USE_CALL                0
#define USE_SMS                 0
#define USE_SYS_LED             1
#define USE_DATA_LED            1
#define USE_LOW_POW_MODE        0
#define USE_GPS_FIXED           0
#define USE_INTERACTIVE_LED     1
#define USE_BSNL_CONTEXT        1
#define USE_AIRTEL_CONTEXT      0


//LED DEFINES
enum LED_DEFINES{
    LED_SYS_ON = 0,
    LED_NO_NETWORK,
    LED_NO_GPS,
    LED_NO_FIX
};

//BSNL Context
Network_PDP_Context_t bsnl_context = {
                .apn        ="bsnlnet",  //airtelgprs.com  -> For Airtel    //bsnlnet  -> For BSNL
                .userName   = ""    ,
                .userPasswd = ""
            };
//Airtel Context
Network_PDP_Context_t airtel_context = {
                .apn        ="airtelgprs.com",  //airtelgprs.com  -> For Airtel    //bsnlnet  -> For BSNL
                .userName   = ""    ,
                .userPasswd = ""
            };



// const uint8_t nmea[]="$GNGGA,000021.263,2228.7216,N,11345.5625,E,0,0,,153.3,M,-3.3,M,,*4E\r\n$GPGSA,A,1,,,,,,,,,,,,,,,*1E\r\n$BDGSA,A,1,,,,,,,,,,,,,,,*0F\r\n$GPGSV,1,1,00*79\r\n$BDGSV,1,1,00*68\r\n$GNRMC,000021.263,V,2228.7216,N,11345.5625,E,0.000,0.00,060180,,,N*5D\r\n$GNVTG,0.00,T,,M,0.000,N,0.000,K,N*2C\r\n";

void EventDispatch(API_Event_t* pEvent)
{
    switch(pEvent->id)
    {
        case API_EVENT_ID_NO_SIMCARD:
            #if USE_TRACE
            Trace(10,"!!NO SIM CARD%d!!!!",pEvent->param1);
            #endif
            networkFlag = false;
            #if USE_INTERACTIVE_LED
            led_status = LED_NO_NETWORK;
            #endif
            break;
        case API_EVENT_ID_NETWORK_REGISTER_SEARCHING:
            #if USE_TRACE
            Trace(2,"network register searching");
            #endif
            networkFlag = false;
            #if USE_INTERACTIVE_LED
            led_status = LED_NO_NETWORK;
            #endif
            break;
        case API_EVENT_ID_NETWORK_REGISTER_DENIED:
            #if USE_TRACE
            Trace(2,"network register denied");
            #endif
        case API_EVENT_ID_NETWORK_REGISTER_NO:
            #if USE_TRACE
            Trace(2,"network register no");
            #endif
            break;
        case API_EVENT_ID_GPS_UART_RECEIVED:
            // Trace(1,"received GPS data,length:%d, data:%s,flag:%d",pEvent->param1,pEvent->pParam1,flag);
            GPS_Update(pEvent->pParam1,pEvent->param1);
            break;
        case API_EVENT_ID_NETWORK_REGISTERED_HOME:
        case API_EVENT_ID_NETWORK_REGISTERED_ROAMING:
        {
            uint8_t status;
            #if USE_TRACE
            Trace(2,"network register success");
            #endif
            bool ret = Network_GetAttachStatus(&status);
            if(!ret)
            {
                #if USE_TRACE
                Trace(1,"get attach staus fail");
                #endif
            }
            #if USE_TRACE
            Trace(1,"attach status:%d",status);
            #endif
            if(status == 0)
            {
                ret = Network_StartAttach();
                if(!ret)
                {
                    #if USE_TRACE
                    Trace(1,"network attach fail");
                    #endif
                }
            }
            else
            {
                #if USE_BSNL_CONTEXT
                Network_StartActive(bsnl_context);
                #endif
                #if USE_AIRTEL_CONTEXT
                Network_StartActive(airtel_context);
                #endif
                // Network_PDP_Context_t context = {
                //     .apn        ="bsnlnet",  //airtelgprs.com  -> For Airtel    //bsnlnet  -> For BSNL
                //     .userName   = ""    ,
                //     .userPasswd = ""
                // };
                // Network_StartActive(context);
            }
            break;
        }
        case API_EVENT_ID_NETWORK_ATTACHED:
            #if USE_TRACE
            Trace(2,"network attach success");
            #endif
            #if USE_BSNL_CONTEXT
                Network_StartActive(bsnl_context);
                #endif
                #if USE_AIRTEL_CONTEXT
                Network_StartActive(airtel_context);
                #endif
            // Network_PDP_Context_t context = {
            //     .apn        ="bsnlnet",  //airtelgprs.com  -> For Airtel    //bsnlnet  -> For BSNL
            //     .userName   = ""    ,
            //     .userPasswd = ""
            // };
            // Network_StartActive(context);
           
            break;

        case API_EVENT_ID_NETWORK_ACTIVATED:
            #if USE_TRACE
            Trace(2,"network activate success");
            #endif
            networkFlag = true;
            break;
        
        case API_EVENT_ID_UART_RECEIVED:
            if(pEvent->param1 == UART1)
            {
                uint8_t data[pEvent->param2+1];
                data[pEvent->param2] = 0;
                memcpy(data,pEvent->pParam1,pEvent->param2);
                #if USE_TRACE
                Trace(1,"uart received data,length:%d,data:%s",pEvent->param2,data);
                #endif
                if(strcmp(data,"close") == 0)
                {
                    #if USE_TRACE
                    Trace(1,"close gps");
                    #endif
                    GPS_Close();
                    isGpsOn = false;
                }
                else if(strcmp(data,"open") == 0)
                {
                    #if USE_TRACE
                    Trace(1,"open gps");
                    #endif
                    GPS_Open(NULL);
                    isGpsOn = true;
                }
            }
            break;
        case API_EVENT_ID_KEY_DOWN:
            // Trace(1,"key down, key:0x%02x",pEvent->param1);
            if(pEvent->param1 == KEY_POWER)
            {
                #if USE_TRACE
                Trace(1,"power key press down now");
                #endif
            }
            break;

        #if USE_CALL
        case API_EVENT_ID_CALL_HANGUP:  //param1: is remote release call, param2:error code(CALL_Error_t)
            #if USE_TRACE
            Trace(1,"Hang up,is remote hang up:%d, error code:%d",pEvent->param1,pEvent->param2);
            #endif
            break;
        case API_EVENT_ID_CALL_INCOMING:   //param1: number type, pParam1:number
            #if USE_TRACE
            Trace(1,"Receive a call, number:%s, number type:%d",pEvent->pParam1,pEvent->param1);
            #endif
            OS_Sleep(5000);
            if(!CALL_Answer())
            {
                #if USE_TRACE
                Trace(1,"answer fail");
                #endif
            }
            break;
        case API_EVENT_ID_CALL_ANSWER  :  
            #if USE_TRACE
            Trace(1,"answer success");
            #endif
            break;
        case API_EVENT_ID_CALL_DTMF    :  //param1: key
            #if USE_TRACE
            Trace(1,"received DTMF tone:%c",pEvent->param1);
            #endif
            break;
        #endif

        #if USE_SMS
         case API_EVENT_ID_SMS_RECEIVED:
            #if USE_TRACE
            Trace(2,"received message");
            #endif
            SMS_Encode_Type_t encodeType = pEvent->param1;
            uint32_t contentLength = pEvent->param2;
            uint8_t* header = pEvent->pParam1;
            uint8_t* content = pEvent->pParam2;

            #if USE_TRACE
            Trace(2,"message header:%s",header);
            Trace(2,"message content length:%d",contentLength);
            #endif
            if(encodeType == SMS_ENCODE_TYPE_ASCII)
            {
                #if USE_TRACE
                Trace(2,"message content:%s",content);
                #endif
                #if USE_UART
                UART_Write(UART1,content,contentLength);
                #endif
            }
            else
            {
                uint8_t tmp[500];
                memset(tmp,0,500);
                for(int i=0;i<contentLength;i+=2)
                {
                    sprintf(tmp+strlen(tmp),"\\u%02x%02x",content[i],content[i+1]);
                }
                #if USE_TRACE
                Trace(2,"message content(unicode):%s",tmp);//you can copy this string to http://tool.chinaz.com/tools/unicode.aspx and display as Chinese
                #endif
                uint8_t* gbk = NULL;
                uint32_t gbkLen = 0;
                if(!SMS_Unicode2LocalLanguage(content,contentLength,CHARSET_CP936,&gbk,&gbkLen))
                {
                    #if USE_TRACE
                    Trace(10,"convert unicode to GBK fail!");
                    #endif
                }
                else
                {
                    memset(tmp,0,500);
                    for(int i=0;i<gbkLen;i+=2)
                    {
                        sprintf(tmp+strlen(tmp),"%02x%02x ",gbk[i],gbk[i+1]);
                    }
                    #if USE_TRACE
                    Trace(2,"message content(GBK):%s",tmp);//you can copy this string to http://m.3158bbs.com/tool-54.html# and display as Chinese
                    #endif
                    #if USE_UART
                    UART_Write(UART1,gbk,gbkLen);//use serial tool that support GBK decode if have Chinese, eg: https://github.com/Neutree/COMTool
                    #endif
                }
                OS_Free(gbk);
            }
            break;
        case API_EVENT_ID_SMS_ERROR:
            #if USE_TRACE
            Trace(10,"SMS error occured! cause:%d",pEvent->param1);
            #endif
            break;
        #endif
        default:
            break;
    }
}

#if USE_VIBRATION_SENSOR
void OnMotion(GPIO_INT_callback_param_t* param)
{
     #if USE_TRACE
    Trace(1,"HIT HIT HIT");
    //close_interrupt = true;
    //motion = true;
    #endif
    //The best option it seems is to restart the system
    if (motion == false && init_done_flag == true 
    #if USE_GPS_FIXED
    && is_Fixed_flag == true
    #endif
    )
    {
        #if USE_TRACE
        Trace(2, "Restart Flag ON");
        #endif
        restart_flag = true;
    }
}
#endif

#if USE_MPU_SENSOR
void OnMotion_MPU(GPIO_INT_callback_param_t* param)
{
    #if USE_TRACE
    Trace(1,"MPU HIT HIT");
    #endif
#if USE_UART
    UART_Write(UART1, "MPU HIT HIT", strlen("MPU HIT HIT"));
#endif
}

bool MPU_Init()
{
    uint8_t accId;
	uint8_t Data;
    I2C_Config_t config;

    config.freq = I2C_FREQ_100K;
    I2C_Init(I2C_ACC, config);

    //I2C enum - 0 means no error, any other value is error
    //read accelerator chip ID: 0x33
        if(I2C_ReadMem(I2C_ACC, 0x68, 0x75, 1, &accId, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }

        //Who Am I read of sensor
        #if USE_TRACE
        Trace(1,"accelerator id shold be 0x76, read:0X%02x",accId);
        #endif

        OS_Sleep(3000);
		
        //MPU Code for enabling Motion Interrupt
		Data = 0x00;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, 0x6B, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        //Reset all internal signal paths in the MPU-6050 by writing 0x07 to register 0x68;
		Data = 0x07;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, SIGNAL_PATH_RESET, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        //write register 0x37 to select how to use the interrupt pin. For an active high, push-pull signal that stays until register (decimal) 58 is read, write 0x20.
		Data = 0x20;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, I2C_SLV0_ADDR, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        //Write register 28 (==0x1C) to set the Digital High Pass Filter, bits 3:0. For example set it to 0x01 for 5Hz. (These 3 bits are grey in the data sheet, but they are used! Leaving them 0 means the filter always outputs 0.)
		Data = 0x01;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, ACCEL_CONFIG, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        //Write the desired Motion threshold to register 0x1F (For example, write decimal 20). 
		Data = 10;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, MOT_THR, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        //Set motion detect duration to 1  ms; LSB is 1 ms @ 1 kHz rate  
		Data = 40;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, MOT_DUR, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        //to register 0x69, write the motion detection decrement and a few other settings (for example write 0x15 to set both free-fall and motion decrements to 1 and accelerometer start-up delay to 5ms total by adding 1ms. ) 
		Data = 0x15;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, MOT_DETECT_CTRL, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
		//write register 0x38, bit 6 (0x40), to enable motion detection interrupt. 
        Data = 0x40;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, INT_ENABLE, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
		
        // now INT pin is active low
		Data = 160;
		if(I2C_WriteMem(I2C_ACC, MPU_ADDRESS, 0x37, 1, &Data, 1, I2C_DEFAULT_TIME_OUT))
        {
            //Error
            return false;
        }
	
	 GPIO_config_t gpioInt_MPU = {
        .mode               = GPIO_MODE_INPUT_INT,
        .pin                = GPIO_PIN3,
        .defaultLevel       = GPIO_LEVEL_HIGH,
        .intConfig.debounce = 50,
        .intConfig.type     = GPIO_INT_TYPE_FALLING_EDGE,  //Active Low
        .intConfig.callback = OnMotion_MPU
    };
	
	 if(!GPIO_Init(gpioInt_MPU))
     {
        return false;
     }

    return true;

}
#endif


# if USE_LOW_POW_MODE
void low_power_mode()
{
     #if USE_TRACE
    Trace(2,"Entering Low Power Mode");
    #endif
    //Disable Power to Peripherals

    //Close GPS
    GPS_Close();
    isGpsOn = false;

    //Flight Mode 
    Network_SetFlightMode(true);

    //
}
#endif

#if USE_SMS
void SMSInit()
{
    if(!SMS_SetFormat(SMS_FORMAT_TEXT,SIM0))
    {
        #if USE_TRACE
        Trace(1,"sms set format error");
        #endif
        return;
    }
    SMS_Parameter_t smsParam = {
        .fo = 17 ,
        .vp = 167,
        .pid= 0  ,
        .dcs= 8  ,//0:English 7bit, 4:English 8 bit, 8:Unicode 2 Bytes
    };
    if(!SMS_SetParameter(&smsParam,SIM0))
    {
        #if USE_TRACE
        Trace(1,"sms set parameter error");
        #endif
        return;
    }
    if(!SMS_SetNewMessageStorage(SMS_STORAGE_SIM_CARD))
    {
        #if USE_TRACE
        Trace(1,"sms set message storage fail");
        #endif
        return;
    }
}
#endif

//http post with no header
int Http_Post(const char* domain, int port,const char* path,uint8_t* body, uint16_t bodyLen, char* retBuffer, int bufferLen)
{
    uint8_t ip[16];
    bool flag = false;
    uint16_t recvLen = 0;

    //connect server
    memset(ip,0,sizeof(ip));
    if(DNS_GetHostByName2(domain,ip) != 0)
    {
         #if USE_TRACE
        Trace(2,"get ip error");
        #endif
        return -1;
    }
    // Trace(2,"get ip success:%s -> %s",domain,ip);
    char* servInetAddr = ip;
    char* temp = OS_Malloc(2048);
    if(!temp)
    {
         #if USE_TRACE
        Trace(2,"malloc fail");
        #endif
        return -1;
    }
    snprintf(temp,2048,"POST %s HTTP/1.1\r\nContent-Type: application/x-www-form-urlencoded\r\nConnection: Keep-Alive\r\nHost: %s\r\nContent-Length: %d\r\n\r\n",
                            path,domain,bodyLen);
    char* pData = temp;
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if(fd < 0){
         #if USE_TRACE
        Trace(2,"socket fail");
        #endif
        OS_Free(temp);
        return -1;
    }
    // Trace(2,"fd:%d",fd);

    struct sockaddr_in sockaddr;
    memset(&sockaddr,0,sizeof(sockaddr));
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);
    inet_pton(AF_INET,servInetAddr,&sockaddr.sin_addr);

    int ret = connect(fd, (struct sockaddr*)&sockaddr, sizeof(struct sockaddr_in));
    if(ret < 0){
         #if USE_TRACE
        Trace(2,"socket connect fail");
        #endif
        OS_Free(temp);
        close(fd);
        return -1;
    }
    // Trace(2,"socket connect success");
     #if USE_TRACE
    Trace(2,"send request:%s",pData);
    #endif
    ret = send(fd, pData, strlen(pData), 0);
    if(ret < 0){
         #if USE_TRACE
        Trace(2,"socket send fail");
        #endif
        OS_Free(temp);
        close(fd);
        return -1;
    }
    ret = send(fd, body, bodyLen, 0);
    if(ret < 0){
         #if USE_TRACE
        Trace(2,"socket send fail");
        #endif
        OS_Free(temp);
        close(fd);
        return -1;
    }
    // Trace(2,"socket send success");

    struct fd_set fds;
    struct timeval timeout={12,0};
    FD_ZERO(&fds);
    FD_SET(fd,&fds);
    while(!flag)
    {
        ret = select(fd+1,&fds,NULL,NULL,&timeout);
        // Trace(2,"select return:%d",ret);
        switch(ret)
        {
            case -1:
                 #if USE_TRACE
                Trace(2,"select error");
                #endif
                flag = true;
                break;
            case 0:
                 #if USE_TRACE
                Trace(2,"select timeout");
                #endif
                flag = true;
                break;
            default:
                if(FD_ISSET(fd,&fds))
                {
                    memset(retBuffer,0,bufferLen);
                    ret = recv(fd,retBuffer,bufferLen,0);
                    recvLen += ret;
                    if(ret < 0)
                    {
                         #if USE_TRACE
                        Trace(2,"recv error");
                        #endif
                        flag = true;
                        break;
                    }
                    else if(ret == 0)
                    {
                         #if USE_TRACE
                        Trace(2,"ret == 0");
                        #endif
                        break;
                    }
                    else if(ret < 1352)
                    {
                        GPS_DEBUG_I("recv len:%d,data:%s",recvLen,retBuffer);
                        close(fd);
                        OS_Free(temp);
                        return recvLen;
                    }
                }
                break;
        }
    }
    close(fd);
    OS_Free(temp);
    return -1;
}

uint8_t buffer[1024],buffer2[400];

void gps_testTask(void *pData)
{
    GPS_Info_t* gpsInfo = Gps_GetInfo();



    #if USE_UART
    UART_Write(UART1,"Init now\r\n",strlen("Init now\r\n"));
    #endif

    #if USE_WATCHDOG
    //180 Second Watch Dog
    #if USE_TRACE
    Trace(1,"start watchdog with 180 seconds delay");
    #endif
    WatchDog_Open(WATCHDOG_SECOND_TO_TICK(180));
    #endif


    //wait for gprs register complete
    //The process of GPRS registration network may cause the power supply voltage of GPS to drop,
    //which resulting in GPS restart.
    while(!networkFlag)
    {
        #if USE_TRACE
        Trace(1,"wait for gprs regiter complete");
        #endif
        #if USE_UART
        UART_Write(UART1,"wait for gprs regiter complete\r\n",strlen("wait for gprs regiter complete\r\n"));
        #endif
        OS_Sleep(2000);
    }

    //open GPS hardware(UART2 open either)
    GPS_Init();
    GPS_SaveLog(true,GPS_NMEA_LOG_FILE_PATH);
    // if(!GPS_ClearLog())
    //     Trace(1,"open file error, please check tf card");
    GPS_Open(NULL);

    //wait for gps start up, or gps will not response command
    while(gpsInfo->rmc.latitude.value == 0)
        OS_Sleep(1000);
    

    // set gps nmea output interval
    for(uint8_t i = 0;i<5;++i)
    {
        bool ret = GPS_SetOutputInterval(10000);
        #if USE_TRACE
        Trace(1,"set gps ret:%d",ret);
        #endif
        if(ret)
            break;
        OS_Sleep(1000);
    }

    // if(!GPS_ClearInfoInFlash())
    //     Trace(1,"erase gps fail");
    
    // if(!GPS_SetQzssOutput(false))
    //     Trace(1,"enable qzss nmea output fail");

    // if(!GPS_SetSearchMode(true,false,true,false))
    //     Trace(1,"set search mode fail");

    // if(!GPS_SetSBASEnable(true))
    //     Trace(1,"enable sbas fail");
    
    if(!GPS_GetVersion(buffer,150))
    {
        #if USE_TRACE
        Trace(1,"get gps firmware version fail");
        #endif
    }
    else
    {
        #if USE_TRACE
        Trace(1,"gps firmware version:%s",buffer);
        #endif
    }

    // if(!GPS_SetFixMode(GPS_FIX_MODE_LOW_SPEED))
        // Trace(1,"set fix mode fail");
    
    if(!GPS_SetLpMode(GPS_LP_MODE_SUPPER_LP))
    {
        #if USE_TRACE
        Trace(1,"set gps lp mode fail");
        #endif
    }

    if(!GPS_SetOutputInterval(1000))
    {
        #if USE_TRACE
        Trace(1,"set nmea output interval fail");
        #endif
    }
    
    #if USE_TRACE
    Trace(1,"init ok");
    #endif

    init_done_flag = true;
    #if USE_UART
    UART_Write(UART1,"Init ok\r\n",strlen("Init ok\r\n"));
    #endif

    while(1)
    {
        if(isGpsOn)
        {
            //show fix info
            uint8_t isFixed = gpsInfo->gsa[0].fix_type > gpsInfo->gsa[1].fix_type ?gpsInfo->gsa[0].fix_type:gpsInfo->gsa[1].fix_type;
            char* isFixedStr;            
            if(isFixed == 2)
            {
                isFixedStr = TWO_D_FIX;
                is_Fixed_flag = true;
            }
            else if(isFixed == 3)
            {
                if(gpsInfo->gga.fix_quality == 1)
                {
                    isFixedStr = THREE_D_FIX;
                }
                else if(gpsInfo->gga.fix_quality == 2)
                {
                    isFixedStr = DIFF_GPS_FIX;
                }
                
                is_Fixed_flag = true;
            }
            else
            {
                isFixedStr = NO_FIX;
                is_Fixed_flag = false;

                #if USE_GPS_FIXED
                #if USE_INTERACTIVE_LED
                led_status = LED_NO_FIX;
                #endif
                #endif
            }

            //convert unit ddmm.mmmm to degree(°) 
            int temp = (int)(gpsInfo->rmc.latitude.value/gpsInfo->rmc.latitude.scale/100);
            double latitude = temp+(double)(gpsInfo->rmc.latitude.value - temp*gpsInfo->rmc.latitude.scale*100)/gpsInfo->rmc.latitude.scale/60.0;
            temp = (int)(gpsInfo->rmc.longitude.value/gpsInfo->rmc.longitude.scale/100);
            double longitude = temp+(double)(gpsInfo->rmc.longitude.value - temp*gpsInfo->rmc.longitude.scale*100)/gpsInfo->rmc.longitude.scale/60.0;

            
            //you can copy ` latitude,longitude ` to http://www.gpsspg.com/maps.htm check location on map

            snprintf(buffer,sizeof(buffer),"GPS fix mode:%d, BDS fix mode:%d, fix quality:%d, satellites tracked:%d, gps sates total:%d, is fixed:%s, coordinate:WGS84, Latitude:%f, Longitude:%f, unit:degree,altitude:%f",gpsInfo->gsa[0].fix_type, gpsInfo->gsa[1].fix_type,
                                                                gpsInfo->gga.fix_quality,gpsInfo->gga.satellites_tracked, gpsInfo->gsv[0].total_sats, isFixedStr, latitude,longitude,gpsInfo->gga.altitude);
            #if USE_TRACE
            //show in tracer
            Trace(1,buffer);
            #endif

            //send to UART1
            #if USE_UART
            UART_Write(UART1,buffer,strlen(buffer));
            UART_Write(UART1,"\r\n\r\n",4);
            #endif
            char* requestPath = buffer2;
            uint8_t percent;
            uint16_t v = PM_Voltage(&percent);
            #if USE_TRACE
            Trace(1,"power:%d %d",v,percent);
            #endif

            memset(buffer,0,sizeof(buffer));
            if(!INFO_GetIMEI(buffer))
                Assert(false,"NO IMEI");

            #if USE_TRACE
            Trace(1,"device name:%s",buffer);
            #endif
            #if USE_UART
            UART_Write(UART1,buffer,strlen(buffer));
            #endif
            snprintf(requestPath,sizeof(buffer2),"/?id=%s&timestamp=%d&lat=%f&lon=%f&speed=%f&bearing=%.1f&altitude=%f&accuracy=%.1f&batt=%.1f",
                                                    buffer,time(NULL),latitude,longitude,isFixed*1.0,0.0,gpsInfo->gga.altitude,0.0,percent*1.0);
            //Check False GPS
            if(prev_lat > 0.0 && prev_long > 0.0)
            {
                if(((abs(latitude - prev_lat)*GPS_MUL_FACTOR_MAX) < LAT_THRESHOLD_MAX)  && ((abs(longitude - prev_long)*GPS_MUL_FACTOR_MAX) < LONG_THRESHOLD_MAX)
                  //  && ((abs(latitude - prev_lat)*GPS_MUL_FACTOR_MIN) > LAT_THRESHOLD_MIN)  && ((abs(longitude - prev_long)*GPS_MUL_FACTOR_MIN) > LONG_THRESHOLD_MIN)
                    )
                {
                    falseGPS = false;
                }
                else
                {
                    falseGPS = true;
                }
                if((latitude-prev_lat == 0.0) && (longitude-prev_long == 0.0) 
                #if USE_GPS_FIXED 
                && is_Fixed_flag == true
                #endif
                )
                {
                    motion = false;     //Tracker not moving
                }
                else
                {
                    motion = true;      // Tracker Moving
                }
            }
            
            uint8_t status;
            Network_GetActiveStatus(&status);


            //If network internet is there status == 1
            //If no falseGPS signal received 
            //If GPS is Fixed (i.e should not be equal to NO_FIX)
            //Only then send data to HTTP server
            if(status && !falseGPS 
            #if USE_GPS_FIXED 
                && is_Fixed_flag == true
                #endif 
            )
            {
                #if USE_DATA_LED
                GPIO_Set(UPLOAD_DATA_LED,GPIO_LEVEL_HIGH);
                #endif

                if(Http_Post(SERVER_IP,SERVER_PORT,requestPath,NULL,0,buffer,sizeof(buffer)) <0 )
                {
                    #if USE_TRACE
                    Trace(1,"send location to server fail");
                    #endif
                }
                    
                else
                {
                    #if USE_TRACE
                    Trace(1,"send location to server success");
                    Trace(1,"response:%s",buffer);
                    #endif

                }

                #if USE_DATA_LED
                GPIO_Set(UPLOAD_DATA_LED,GPIO_LEVEL_LOW);
                #endif
            }
            else
            {
                #if USE_TRACE
                Trace(1,"no internet OR False GPS OR No Fix");
                #endif
            }

            //Load the prev variables 
            prev_lat = latitude;
            prev_long = longitude;
        }

        #if USE_WATCHDOG
        WatchDog_KeepAlive();  // Feed  the Watchdog
        #endif

        PM_SetSysMinFreq(PM_SYS_FREQ_32K);
        if(motion == true)
        {
            OS_Sleep(2000); //2 Second Gap
        }
        else
        {
            #if USE_LOW_POW_MODE
            low_pow_mode_flag = true;
            #endif

            OS_Sleep(60000*2);  //2 min gap
        }
        PM_SetSysMinFreq(PM_SYS_FREQ_178M);
    }
}

#if USE_SYS_LED
void LED_Blink(void* param)
{
    static int count = 0;
    #if USE_INTERACTIVE_LED
    switch (led_status)
    {
    case LED_SYS_ON:
        /* LED On for 10 seconds  */
        if (++count == 10)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_HIGH);
        }
        else if (count == 11)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_LOW);
            count = 0;
        }
        break;
    case LED_NO_NETWORK:
    /* LED On for 2 seconds  */
        if (++count == 2)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_HIGH);
        }
        else if (count == 3)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_LOW);
            count = 0;
        }
        break;

    case LED_NO_GPS:
    /* LED On for 10 seconds  */
        if (++count == 4)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_HIGH);
        }
        else if (count == 5)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_LOW);
            count = 0;
        }
        break;
    
    case LED_NO_FIX:
    #if USE_GPS_FIXED
        /* LED On for 10 seconds  */
        if (++count == 8)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_HIGH);
        }
        else if (count == 9)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_LOW);
            count = 0;
        }
    #endif
        break;
    
    default:
        break;
    }
    #else
    if (++count == 10)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_HIGH);
        }
        else if (count == 11)
        {
            GPIO_Set(SYSTEM_STATUS_LED, GPIO_LEVEL_LOW);
            count = 0;
        }
    #endif

    
    OS_StartCallbackTimer(gpsTaskHandle,1000,LED_Blink,NULL);
}
#endif

void gps_MainTask(void *pData)
{
    API_Event_t* event=NULL;
    
    TIME_SetIsAutoUpdateRtcTime(true);
    
    #if USE_UART
    //open UART1 to print NMEA infomation
    UART_Config_t config = {
        .baudRate = UART_BAUD_RATE_115200,
        .dataBits = UART_DATA_BITS_8,
        .stopBits = UART_STOP_BITS_1,
        .parity   = UART_PARITY_NONE,
        .rxCallback = NULL,
        .useEvent   = true
    };
    #endif
    #if USE_SYS_LED
    GPIO_config_t gpioLedBlue = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin          = SYSTEM_STATUS_LED,
        .defaultLevel = GPIO_LEVEL_LOW
    };
    #endif
    #if USE_DATA_LED
    GPIO_config_t gpioLedUpload = {
        .mode         = GPIO_MODE_OUTPUT,
        .pin          = UPLOAD_DATA_LED,
        .defaultLevel = GPIO_LEVEL_LOW
    };
    #endif
    // #if USE_VIBRATION_SENSOR
    // GPIO_config_t gpioInt = {
    //     .mode               = GPIO_MODE_INPUT_INT,
    //     .pin                = VIBRATION_SENS_INPUT,
    //     .defaultLevel       = GPIO_LEVEL_LOW,
    //     .intConfig.debounce = 50,
    //     .intConfig.type     = GPIO_INT_TYPE_RISING_EDGE,
    //     .intConfig.callback = OnMotion
    // };
    // #endif

    PM_PowerEnable(POWER_TYPE_VPAD,true);
    #if USE_SYS_LED
    GPIO_Init(gpioLedBlue);
    #endif
    #if USE_DATA_LED
    GPIO_Init(gpioLedUpload);
    #endif
    // #if USE_VIBRATION_SENSOR
    // GPIO_Init(gpioInt);
    // #endif
    #if USE_UART
    UART_Init(UART1,config);
    #endif
    #if USE_SMS
    SMSInit();
    #endif
    #if USE_MPU_SENSOR
    MPU_Init();
    #endif

    //Create UART1 send task and location print task
    OS_CreateTask(gps_testTask,
            NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    
    #if USE_SYS_LED
    OS_StartCallbackTimer(gpsTaskHandle,1000,LED_Blink,NULL);
    #endif
    //Wait event
    while(1)
    {
        if(OS_WaitEvent(gpsTaskHandle, (void**)&event, OS_TIME_OUT_WAIT_FOREVER))
        {
            EventDispatch(event);
            OS_Free(event->pParam1);
            OS_Free(event->pParam2);
            OS_Free(event);
        }
        if(restart_flag == true)
        {
            PM_Reboot();
        }
        #if USE_LOW_POW_MODE
        if(low_pow_mode_flag == true)
        {
            low_power_mode();
        }
        #endif

        #if USE_INTERACTIVE_LED
        if(isGpsOn == true 
        #if USE_GPS_FIXED
        && is_Fixed_flag == true
        #endif
         && networkFlag == true)
        {
            led_status = LED_SYS_ON;
        }
        
        #endif

        #if USE_VIBRATION_SENSOR
        switch (motion)
        {
        case true:
            // if (close_interrupt == true)
            // {
            //     GPIO_Close(VIBRATION_SENS_INPUT);

            //     GPIO_config_t gpioInt_cls = {
            //         .mode = GPIO_MODE_OUTPUT,
            //         .pin = VIBRATION_SENS_INPUT,
            //         .defaultLevel = GPIO_LEVEL_LOW,
            //     };

            //     GPIO_Init(gpioInt_cls);
            //     int_status = false;
            //     close_interrupt = false;
            // }

            break;
        case false:
            if(int_status == false)
            {
                int_status = true;
                GPIO_Close(VIBRATION_SENS_INPUT);

                GPIO_config_t gpioInt_opn = {
                    .mode = GPIO_MODE_INPUT_INT,
                    .pin = VIBRATION_SENS_INPUT,
                    .defaultLevel = GPIO_LEVEL_LOW,
                    .intConfig.debounce = 50,
                    .intConfig.type = GPIO_INT_TYPE_RISING_EDGE,
                    .intConfig.callback = OnMotion
                    };
                GPIO_Init(gpioInt_opn);
            }
            break;
        
        default:
            break;
        }
    
        #endif
    }
}


void app_Main(void)
{
    gpsTaskHandle = OS_CreateTask(gps_MainTask,
        NULL, NULL, MAIN_TASK_STACK_SIZE, MAIN_TASK_PRIORITY, 0, 0, MAIN_TASK_NAME);
    OS_SetUserMainHandle(&gpsTaskHandle);
}

