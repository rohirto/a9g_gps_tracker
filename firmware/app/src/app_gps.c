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
#include "app_gps.h"

bool isGpsOn = true;
volatile bool falseGPS = true;
volatile double prev_lat = 0.0;
volatile double prev_long = 0.0;