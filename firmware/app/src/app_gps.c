/**
 * @file app_gps.c
 * @author rohirto
 * @brief GPS module
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#include "app.h"
#include "app_gps.h"
#include "app_gprs.h"
#include "app_accl.h"

bool isGpsOn = true;
volatile bool falseGPS = true;
volatile double prev_lat = 0.0;
volatile double prev_long = 0.0;

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
