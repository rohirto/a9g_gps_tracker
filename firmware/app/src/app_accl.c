/**
 * @file app_accl.c
 * @author rohirto
 * @brief Acclerometer Module of the Application
 * @version 0.1
 * @date 2024-03-13
 * 
 * @copyright Copyright (c) 2024
 * 
 */

#include "app_accl.h"

volatile bool motion = true;   //Taking initial motion true, this logic is needed for Interrupt based restart fucntion

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
