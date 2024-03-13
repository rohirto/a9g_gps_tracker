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

//Macros
#define GPS_NMEA_LOG_FILE_PATH "/t/gps_nmea.log"


//Externs
extern bool isGpsOn;
extern volatile bool falseGPS;
extern volatile double prev_lat;
extern volatile double prev_long;

#endif