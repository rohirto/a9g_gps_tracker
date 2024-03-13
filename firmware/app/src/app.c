
/**
 * @file app.c
 * @author rohirto
 * @brief A9G Based GPS Tracking Module - Application layer
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "app.h"
#include "app_accl.h"
#include "app_gprs.h"
#include "app_gps.h"

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

//Macros
#define MAIN_TASK_STACK_SIZE    (2048 * 2)
#define MAIN_TASK_PRIORITY      0
#define MAIN_TASK_NAME          "GPS Test Task"

static HANDLE gpsTaskHandle = NULL;



volatile bool restart_flag = false;
volatile bool init_done_flag = false;
volatile bool is_Fixed_flag = false;
volatile bool low_pow_mode_flag = false;
volatile uint8_t led_status;
//volatile bool close_interrupt = false;
volatile bool int_status = false;



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


//http post with no header


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

