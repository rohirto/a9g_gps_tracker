/**
 * @file app_gprs.h
 * @author rohirto
 * @brief Header file for GPRS File
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef APP_GPRS_H
#define APP_GPRS_H

#include <stdbool.h>
#include "api_call.h"
#include "api_sms.h"
#include "api_socket.h"
#include "api_network.h"
#include "api_info.h"

//Macros
#define SERVER_IP   "demo3.traccar.org"
#define SERVER_PORT  5055


//Externs
extern bool networkFlag;
extern Network_PDP_Context_t bsnl_context;
extern Network_PDP_Context_t airtel_context;

//Function Prototypes
int Http_Post(const char* , int ,const char* ,uint8_t* , uint16_t , char* , int );

#endif