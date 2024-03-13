/**
 * @file app.h
 * @author rohirto
 * @brief Main header file
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */
#ifndef __APP_H_
#define __APP_H_

//Includes
#include "api_info.h"
#include "api_hal_pm.h"
#include "time.h"
#include "assert.h"
#include "api_hal_gpio.h"
#include "api_hal_watchdog.h"
#include <api_event.h>
#include <api_hal_uart.h>
#include <api_debug.h>
#include "buffer.h"
#include "math.h"
#include <string.h>
#include <stdio.h>
#include <api_os.h>

//Custumisation
#define USE_WATCHDOG            1
#define USE_TRACE               1
#define USE_UART                1
#define USE_VIBRATION_SENSOR    0
#define USE_MPU_SENSOR          0
#define USE_CALL                0
#define USE_SMS                 0
#define USE_SYS_LED             1
#define USE_DATA_LED            1
#define USE_LOW_POW_MODE        0
#define USE_GPS_FIXED           0
#define USE_INTERACTIVE_LED     1
#define USE_BSNL_CONTEXT        1
#define USE_AIRTEL_CONTEXT      0

#define GPIO_PIN_LED_BLUE   GPIO_PIN27
#define GPIO_PIN_LED_GREEN  GPIO_PIN28   

#define SYSTEM_STATUS_LED       GPIO_PIN27
#define UPLOAD_DATA_LED         GPIO_PIN28


//GPRS Context
#define PDP_CONTEXT_APN       "airtelgprs.com"
#define PDP_CONTEXT_USERNAME  ""
#define PDP_CONTEXT_PASSWD    ""

//Server info
#define SERVER_IPP   "demo3.traccar.org"       //Traccar Server IP
#define SERVER_PORT  5055               //Using OsmAND protocol for this purpose
#define SERVER_ID   "rohirto_1"

//SD Card save file
#define GPS_NMEA_LOG_FILE_PATH "/t/gps_nmea.log"        //When SD Card will come

//GPIOs
#define SYSTEM_STATUS_LED GPIO_PIN27
#define UPLOAD_DATA_LED   GPIO_PIN28


//Const strings
#define INIT_STRING             "#LOG: Init now....\r\n"
#define GPS_VERSION_FAIL        "#LOG: get gps firmware version fail\r\n"
#define GPS_FW_VERSION          "#LOG: gps firmware version: "
#define GPS_STARTING            "#LOG: GPS starting up...\r\n"
#define GPRS_WAIT               "#LOG: wait for gprs regiter complete\r\n"
#define UART_CONFIG_SUCCESS     "#LOG: UART Config: Success\r\n"
#define UART_CONFIG_FAIL        "#LOG: UART Config: Fail\r\n"
#define GPIO_CONFIG_SUCCESS     "#LOG: GPIO Config: Success\r\n"
#define GPIO_CONFIG_FAIL        "#LOG: GPIO Config: Fail\r\n"

//DEBUG level
#define DEBUG_LEVEL_TRACE       1       //Debug level high, Trace 
#define DEBUG_LEVEL_UART        0       //Debug level low, Only UART
#define DEBUG_LEVEL_TRACE_UART  2       //DEbug level high, Trace + UART

#define DEBUG_LEVEL             DEBUG_LEVEL_TRACE_UART

//LED DEFINES
enum LED_DEFINES{
    LED_SYS_ON = 0,
    LED_NO_NETWORK,
    LED_NO_GPS,
    LED_NO_FIX
};

//Externs
extern volatile bool restart_flag ;
extern volatile bool init_done_flag ;
extern volatile bool is_Fixed_flag ;
extern volatile bool low_pow_mode_flag ;
extern volatile uint8_t led_status;
//volatile bool close_interrupt = false;
extern volatile bool int_status ;




/* Func Prototypes */
void httpGetTask(void*);
void GpsTask(void *);
void initTask(void*);

#endif

