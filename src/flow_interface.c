/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited
 * All rights reserved.
 *
 * Redistribution and use of the Software in source and binary forms, with or
 * without modification, are permitted provided that the following conditions are met:
 *
 *     1. The Software (including after any modifications that you make to it) must
 *        support the FlowCloud Web Service API provided by Licensor and accessible
 *        at  http://ws-uat.flowworld.com and/or some other location(s) that we specify.
 *
 *     2. Redistributions of source code must retain the above copyright notice, this
 *        list of conditions and the following disclaimer.
 *
 *     3. Redistributions in binary form must reproduce the above copyright notice, this
 *        list of conditions and the following disclaimer in the documentation and/or
 *        other materials provided with the distribution.
 *
 *     4. Neither the name of the copyright holder nor the names of its contributors may
 *        be used to endorse or promote products derived from this Software without
 *        specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 **************************************************************************************************/

/**
 * @file flow_interface.c
 * @brief Interface functions for initializing libflow, registering as a device using FlowCore SDK.
 *    
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <flow/flowmessaging.h>
#include <libconfig.h>
#include "log.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

/** Max array size of registration data elements. */
#define MAX_SIZE (256)
/** Message expiry timeout on flow cloud. */
#define MESSAGE_EXPIRY_TIMEOUT (20)
/** Configuration file to get registration data stored by provisioning app. */
#define CONFIG_FILE "/etc/lwm2m/flow_access.cfg"
/** Number of trials for reading configuration file */
#define FILE_READ_TRIALS (5)
 
/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/*to hold the device ID*/
char deviceID[40];

/***************************************************************************************************
 * Typedef
 **************************************************************************************************/
 
/**
 * A structure to contain flow registration data.
 */
typedef struct
{
    /*@{*/
    char url[MAX_SIZE]; /**< flow server url */
    char key[MAX_SIZE]; /**< customer authentification key */
    char secret[MAX_SIZE]; /**< customer secret key */
    char deviceName[MAX_SIZE]; /**< registered device name */
    char fcap[MAX_SIZE]; /**< code for provisioning */ 
    char devicetype[MAX_SIZE]; /**<  type of device */
    char serial[MAX_SIZE]; /**< device serial number */
    char software_version[MAX_SIZE]; /**< application version */
    char DevID[MAX_SIZE]; /**< DeviceID of registered device*/
    /*@}*/
}RegistrationData;


/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Get value for corresponding key from the configuration file.
 * @param *cfg pointer to configuration object.
 * @param *dest value string of the key.
 * @param *key key to be searched in configuration file.
 * @return true if configuration read successfuly, else false.
 */
static bool GetValueForKey(config_t *cfg, char *dest, const char *key)
{
    const char *tmp;
    if (config_lookup_string(cfg, key, &tmp) != CONFIG_FALSE)
    {
        strncpy(dest, tmp, MAX_SIZE - 1);
        dest[MAX_SIZE - 1] = '\0';
        return true;
    }
    return false;
}

/**
 * @brief Read device resgitration settings from configuration file.
 *        And if not found, ask user for settings.
 * @param *regData pointer to device registration data.
 * @return true if configuration read successfuly, else false.
 */
static bool GetConfigData(RegistrationData *regData)
{
    config_t cfg;
    bool success = false;
    int i;
    config_init(&cfg);
    for (i = FILE_READ_TRIALS; i > 0; i--)
    {
        if (!config_read_file(&cfg, CONFIG_FILE))
        {
            LOG(LOG_INFO, "Waiting for config data");
            sleep(1);
        }
        else
        {
            if (GetValueForKey(&cfg, regData->url, "URL") &&
                GetValueForKey(&cfg, regData->key, "CustomerKey") &&
                GetValueForKey(&cfg, regData->secret, "CustomerSecret") &&
                GetValueForKey(&cfg, regData->deviceName, "Name") &&
                GetValueForKey(&cfg, regData->fcap, "FCAP") &&
                GetValueForKey(&cfg, regData->devicetype, "DeviceType") &&
                GetValueForKey(&cfg, regData->serial, "SerialNumber") &&
                GetValueForKey(&cfg, regData->software_version, "SoftwareVersion") &&
                GetValueForKey(&cfg, regData->DevID, "DeviceID"))
            {
                success = true;
            }
            else
            {
                LOG(LOG_INFO, "Failed to read config data");
            }
            config_destroy(&cfg);
            break;
        }
    }

    if (i == 0)
    {
        LOG(LOG_INFO, "Failed to read config file");
        config_destroy(&cfg);
    }
    return success;
}

/**
 * @brief Initialize libflowcore and libflowmessaging.
 * @param *url pointer to server url.
 * @param *key pointer to customer authentication key.
 * @param *secret pointer to customer secret key.
 * @param *rememberMeToken remember me token for flow cloud access.
 * @return true if libflow initialization is successful, else false.
 */
static bool InitialiseLibFlow(const char *url,
                              const char *key,
                              const char *secret)
{
    if (FlowCore_Initialise())
    {
        FlowCore_RegisterTypes();operation
        if (FlowMessaging_Initialise())
        {
            if (FlowClient_ConnectToServer(url, key, secret, false))
            {
                return true;
            }
            else
            {
                FlowCore_Shutdown();
                FlowMessaging_Shutdown();
                LOG(LOG_INFO, "Failed to connect to server");
            }
        }
        else
        {
            LOG(LOG_INFO, "Flow Messaging initialization failed");
        }
    }
    else
    {
        LOG(LOG_INFO, "Flow Core initialization failed");
    }
    return false;
}

/**
 * @brief Converts the deviceID from opaqoperationue to string.
 */
void deviceidconversion(char *deviceID_file)
{
    int i=0,j=0;
    char temp;
    int k=0;
    
    while(deviceID_file[i]!='\0')
    {
        if(i==10 | i==16 | i==22)
        {
            j = i;
            while (k < j)
            {
                temp = deviceID_file[k];
                deviceID_file[k] = deviceID_file[j];
                deviceID_file[j] = temp;
                k++;
                j--;
            }
            k=i+2;
        }
        i=i+1;
    }
    
    i=0;
    
    while(i<=22)
    {
        temp=deviceID_file[i];
        deviceID_file[i]=deviceID_file[i+1];
        deviceID_file[i+1]=temp;
        i=i+3;
    }
    
    i=0;
    j=0;
    
    while(deviceID_file[i] !='\0')
    {
        if(deviceID_file[i] != ' ')
        {
            if(isalpha(deviceID_file[i]))
            {
                deviceID_file[j]=deviceID_file[i]+32;
                j=j+1;
            }
            else
            {
                deviceID_file[j]=deviceID_file[i];
                j=j+1;
            }
            i=i+1;
        }
        else
        {
            i=i+1;
        }
    }
    
    deviceID_file[j]='\0';
    i=0;
    j=0;
    
    while(deviceID_file[i]!='\0')
    {
        if(i==7 | i==11 | i==15 | i==19)
        {
            deviceID[j]=deviceID_file[i];
            j=j+1;
            deviceID[j]='-';
        }
        else
        {
            deviceID[j]=deviceID_file[i];
        }
        
        i=i+1;
        j=j+1;
    }
    
    deviceID[j]='\0';
}

/**
 * @brief InitializeAndRegisterFlowDevice initialize libflow and register device with flow cloud. It read
 *        the credentials from flow_access.cfg file and use credentials to login the device.
 * @return true if device is registered successfully, else false.
 */
bool InitializeAndRegisterFlowDevice(void)
{
    RegistrationData regData;
    if (GetConfigData(&regData))
    {
        deviceidconversion(regData.DevID);
        if (InitialiseLibFlow(regData.url, regData.key, regData.secret))
        {
            if(FlowClient_LoginAsDevice(regData.devicetype,NULL, regData.serial,deviceID, regData.software_version,regData.deviceName, regData.fcap))
            {
                LOG(LOG_INFO, "Device registered successfully\n\n");
                return true;
            }
        }
        else
        {
            LOG(LOG_ERR, "Flow Core initialization failed");
        }
    }
    return false;
}

