#ifndef __BLINK_H_
#define __BLINK_H_


#define GPIO_PIN_LED_BLUE   GPIO_PIN27
#define GPIO_PIN_LED_GREEN  GPIO_PIN28   


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




/* Func Prototypes */
void httpGetTask(void*);
void GpsTask(void *);
void initTask(void*);

#endif

