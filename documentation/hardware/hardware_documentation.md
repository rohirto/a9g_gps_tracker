# Hardware Dcoumentation

## Power Supply
1. OBD connector - ![image](https://github.com/rohirto/a9g_gps_tracker/assets/16812616/b234ba2f-ae92-4dbd-98bb-de071925dc06)
The connector is called Euro 5 connector, pinout is given as above.
12V can be obtained from the bike battery.
Basic Test conducted - when key Ignition is on, then between pin 3 and 4, 12V is observed. But when ignition is off then OV is observed

2. Thus when Bike is ON - Power supply needs to be from Bike battery. The Bike battery should also charge the Backup Battery meanwhile
3. When Bike is OFF - Power supply from Backup Battery

## Power Management Circuit
![image](https://github.com/rohirto/a9g_gps_tracker/assets/16812616/a0abf79e-1eb2-4815-9f3e-f1e2d761d6a2)

Found the above Circuit on Github A9G Issues page.
1. Note that SY8086 is a Voltage regulator which converts 5V (USB) to 4.1V for A9G - This regulator IC is already inbuilt in A9G Pudding Dev board
2. In the first diagram - only extra part which is not on the board is D5. Its name is SS34B - It is a Barrier Diode - It is suited for freewheeling, DC/DC converter and reverse polarity protection applications
3. Second diagram regarding LiPo Charger Shared Load is totally external to the A9G board. Its explaination is given as below:
   1. TP4056 - Li-ion Battery charging Module - Already have some exp in using this chip
   2. SI2305 - P channel MOSFET

This circuit ensures the objective stated in point #2 and #3 in Power Supply Section


