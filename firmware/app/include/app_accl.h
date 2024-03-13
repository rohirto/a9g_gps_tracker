/**
 * @file app_accl.h
 * @author rohirto
 * @brief Header file for Acclerometer MOdule
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#ifndef APP_ACCL_H
#define APP_ACCL_H
#include <stdbool.h>
#include "app.h"
#include "api_hal_i2c.h"

//Macros
#define VIBRATION_SENS_INPUT    GPIO_PIN2
#define I2C_ACC                 I2C2

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

//Externs 
extern volatile bool motion;   //Taking initial motion true, this logic is needed for Interrupt based restart fucntion


#endif