# Relay Gateway Application

## Overview
Relay gateway application runs on ci40, which acts as a gateway For Relay click placed on it. Relay gateway application on/off the relays placed on ci40 as per the received notification from the FlowCloud, which contains information of the relay number to be switched on/off. Application also publishes the relay status to device topic "DeviceStatus" after every 60 sec.

Gateway application serves the purpose of:
- It acts as Awalwm2m client to communicate with Awalwm2m server on FlowM2M.

## Prerequisites
Prior to running relay gateway application, make sure that:
- Awalwm2m client daemon(awa_clientd) is running.
- Awalwm2m bootstrap daemon(awa_bootstrapd) is running.
- Gateway Device provisioning is done.

**NOTE:** Please do "ps" on console to see "specific" process is running or not.

## Running Application on Ci40 board
Realy gateway application is getting started as a daemon. Although we could also start it from the command line as :

$ relay_gateway_appd

Output looks something similar to this :

Relay Gateway Application
```
------------------------
```

Client session established


Wait until Gateway device is provisioned


Gateway is provisioned.


Device registered successfully


RELAY 1 configured 

RELAY 2 configured 


DeviceControl Subscribed Successfully


2b01138b-3b53-4ecb-8f8f-61374cd80489|0|/3306/0/5850/1|0|/3306/0/5850/2

Published on DeviceStatus successfully


