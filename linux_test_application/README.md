# Relay Gateway Test Application

## Overview
Relay gateway test application is a linux application which runs on pc and gives user the choices to test the relay gateway application on ci40. it gives user the flexibiliy to choose the relay to be switched on and off and receive notification. upon running it gives option to user, just press the desired option to get the things worked.
flow_access.cfg file is used to get the required fields to get the device provisioned.
just fill in the fiels properly in flow_access.cfg file and put it in /usr directory and then run the application.

## Prerequisites
Prior to running relay gateway test application, make sure that:
```
-flow_access.cfg file is kept in /usr directory.
```
## Running Application on Ci40 board
Realy gateway test Application upon running gives output similar to this :

Relay Gateway Test Application
```
------------------------------
```

FlowMessaging Initialized Successfully...

Connected to FlowServer...

-- List of user's owned devices --
 1) Device name: ci40

Choose a device index to subscribe to as displayed in the above list (1-1): 1

Subscribe Successful

1. Change the Device setting and publish to device topic

2. Exit

1

-- List of user's owned devices --
 1) Device name: ci40

Choose a device index to publish to as displayed in the above list (1-1): 1

Enter the relay number(1/2) : 1

Enter the operation you want to perform(on/off) : off

Publish Event Successful

/********** Received Notification ***********/ 
```
event Topic = DeviceStatus
event TopicDeviceId = 2b01138b-3b53-4ecb-8f8f-61374cd80489
event EventID = 1d9504ff-9c56-4557-895b-5ae8e170d193
event EventTime = Wed Apr 20 07:12:44 2016
event ExpiryTime = Wed Apr 20 07:17:44 2016
content type = application/pidf+xml
content = 2b01138b-3b53-4ecb-8f8f-61374cd80489|1|/3306/0/5850/1|0|/3306/0/5850/2
fromDevice = 2b01138b-3b53-4ecb-8f8f-61374cd80489
```
/**************** end **********************/

1. Change the Device setting and publish to device topic

2. Exit

2
Thank You

