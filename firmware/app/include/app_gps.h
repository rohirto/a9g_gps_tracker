/**
 * @file app_gps.h
 * @author rohirto
 * @brief Header file for GPS module
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef APP_GPS_H
#define APP_GPS_H

#include <stdbool.h>
#include <api_gps.h>
#include "gps.h"
#include "gps_parse.h"


//Macros
#define GPS_NMEA_LOG_FILE_PATH "/t/gps_nmea.log"

#define TWO_D_FIX               "2D Fix"
#define THREE_D_FIX             "3D Fix"
#define DIFF_GPS_FIX            "3D/DGPS fix"
#define NO_FIX                  "no fix"
#define KEY_POWER               0x4B
#define GPS_MUL_FACTOR_MAX      1000
#define GPS_MUL_FACTOR_MIN      1000000
#define LAT_THRESHOLD_MAX       10          //Used to root out false readings, Based on formula (Difference between Extremes lat x MUL FACTOR)
#define LONG_THRESHOLD_MAX      10

#define LAT_THRESHOLD_MIN       4     //Ne
#define LONG_THRESHOLD_MIN      4



//Externs
extern bool isGpsOn;
extern volatile bool falseGPS;
extern volatile double prev_lat;
extern volatile double prev_long;

//Function Prototype
void gps_testTask(void *);


#endif