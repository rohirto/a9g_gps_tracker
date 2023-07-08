# Firmware Documentation 

## Toolchain Setup:
### For Windows platform

1. Follow the steps given here : https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/installation.html
2. Link for CSDTK4.2 is dead in above link, download it from here: https://github.com/ZakKemble/GPRS_C_SDK/releases/tag/v2.129   

![image](https://user-images.githubusercontent.com/16812616/196235398-e553fd67-b6b5-40a5-88c3-71631d420004.png)

3. Restarting the PC once is required after setting up CSDTK4.2
4. For Burn and Debug follow the tutorial here: https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/burn-debug.html


## Firmware Development

A basic blank project can be created and compiled as shown in this tutorial: https://ai-thinker-open.github.io/GPRS_C_SDK_DOC/en/c-sdk/first-code.html

## Some FAQs
1. During Development Phase - Vcc is supplied by USB cable
2. Debugging and Burning the code is done on HSTX, HSRX and GND
   HSTX <---> RX  (Of USB - TTL coverter)
   HSRX <---> TX
   GND  <---> GND

3. If debugging and burning not happening then -
   1. Check RX, TX connections
   2. Check the continuty of the wires
4. Burning and Debugging done through Cooltools - If tracer is not shiwing Debugging messages then stop the tracer and start again

## Basic Run through
1. The application can be found in the folder app
2. In app/src -> app.c is the source file
3. Program entry is at "void app_Main()"
4. In app_Main we create the Maintask and set User Main handle
5. In Main Task, we first configure Debug Uart (This is different from HSTX and HSTX, ie different from Trace function)
6. Then in Main Task a second Task is created -> Here we will do the init of basic stuff and then delete the task
7. Also there is a while(1) loop in Main Task where events are waited for
8. 
