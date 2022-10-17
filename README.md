# a9g_gps_tracker
GPS tracker based on AI thinker A9G Pudding series module

## Description
Vehicle Tracking Project -> Get GPS Data from device and send it to cloud. The mobile app will interactively display device location on map and it's trip.

## Components to be used

1. A9G Pudding Series Development Board 

![image](https://user-images.githubusercontent.com/16812616/196232117-920cb017-c8ec-4a96-94ab-212dcd528246.png)

2. Low power acclerometer to be decided -> Should have an output external interrupt to wake up the m66 controller from deep sleep

3. Power Management System:

      3.1 DC to DC buck converter to be decided: Reqirements -> Powering up A9G Development Board (4.1 V, Peak current requirement 1.6A) Acclerometer (TBD) 
      
      3.2 Back up Battery -> around 15000maH will suffice I think. (LiPo Battery preferred, with BMS and charging mechanism)

## Application Diagram

![m66flow](https://user-images.githubusercontent.com/16812616/196267748-8fb0ad34-0dc2-4109-8bcd-6580fd85f4d2.png)



## Documentation 
Documentation can be found in documentation folder
