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
 * @file  relay_gateway.c
 * @brief relay gateway applicayion on/off the relays placed on ci40 as per the received notification
          from the FlowCloud, which contains information of the relay number to be switched on/off.
          Application also publishes the relay's status after every 60sec. 
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <inttypes.h>
#include "awa/client.h"
#include "flow_interface.h"
#include "flow/core/flow_time.h"
#include "flow/core/flow_memalloc.h"
#include "log.h"
#include <flow/flowcore.h>
#include <flow/flowmessaging.h>
#include "flow/core/flow_object.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

#define IPC_CLIENT_PORT            (12345)
#define IP_ADDRESS                 "127.0.0.1"
#define FLOW_ACCESS_OBJECT_ID      (20001)
#define FLOW_OBJECT_INSTANCE_ID    (0)
#define URL_PATH_SIZE              (16)
#define OPERATION_TIMEOUT          (5000)
#define FLOW_SERVER_CONNECT_TRIALS (5)
#define HEARTBEAT                  (60)
#define MAX_SIZE                   (250)

/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** Set default debug stream to NULL. */
FILE *debugStream = NULL;
/** Set default debug level to info. */
int debugLevel = LOG_INFO;
/** hold the action to be performed on relay(on/off)*/
char operation[10];

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * @brief Prints relay_gateway_appd usage.
 * @param *program holds application name.
 */
static void PrintUsage(const char *program)
{
    printf("Usage: %s [options]\n\n"
            " -l : Log filename.\n"
            " -v : Debug level from 1 to 5\n"
            "      fatal(1), error(2), warning(3), info(4), debug(5) and max(>5)\n"
            "      default is info.\n"
            " -h : Print help and exit.\n\n",
            program);
}

/**
 * @brief Parses command line arguments passed to relay_gateway_appd.
 * @return -1 in case of failure, 0 for printing help and exit, and 1 for success.
 */
static int ParseCommandArgs(int argc, char *argv[], const char **fptr)
{
    int opt, tmp;
    opterr = 0;

    while (1)
    {
        opt = getopt(argc, argv, "l:v:");
        if (opt == -1)
        {
            break;
        }

        switch (opt)
        {
            case 'l':
                *fptr = optarg;
                break;
            case 'v':
                tmp = strtoul(optarg, NULL, 0);
                if (tmp >= LOG_FATAL && tmp <= LOG_DBG)
                {
                    debugLevel = tmp;
                }
                else
                {
                    LOG(LOG_ERR, "Invalid debug level");
                    PrintUsage(argv[0]);
                    return -1;
                }
                break;
            case 'h':
                PrintUsage(argv[0]);
                return 0;
            default:
                PrintUsage(argv[0]);
                return -1;
        }
    }
    
    return 1;
}

/**
 * @breif Exports the required gpio.
 */
void CheckGpio()
{
    system("/root/export_gpio.sh 73 1");
    printf("\n");
    system("/root/export_gpio.sh 8 2");
}

/**
 * @breif Changes the state of the relay as per the notification received.  
 */
void ChangeRelayState(int relay)
{
    if(relay == 1)
    {
        system("echo out > /sys/class/gpio/gpio73/direction");
        if(strcmp(operation, "on") == 0)
        {
            system("echo 1 > /sys/class/gpio/gpio73/value");
        }
        else
        {
            system("echo 0 > /sys/class/gpio/gpio73/value");
        }
    }

    if(relay == 2)
    {
        system("echo out > /sys/class/gpio/gpio8/direction");
        if(strcmp(operation, "on") == 0)
        {
            system("echo 1 > /sys/class/gpio/gpio8/value");
        }
        else
        {
            system("echo 0 > /sys/class/gpio/gpio8/value");
        }
    }
}

/**
 * @breif Parses the xml content of the notification and brings the value for 
 *        relay number and operation, to perform on relay.
 * @param *xml_content holds the xml content of the notification which needs
 *	  to be processed.
 */
int ParseXML(char * xml_content)
{
    int i=0;
    int j=0;
    int relay;
    
    while(xml_content[i] != '\0')
    {
        if(xml_content[i] == '<')
        {
            if(xml_content[i+1] == 'r')
            {
                while(xml_content[i] != '>')
                {
                    i = i + 1;
                }
                relay = xml_content[i+1];
                relay = relay-48;
            }
        }
        
        if(xml_content[i] == '<')
        {
            if(xml_content[i+1] == 'o')
            {
                while(xml_content[i] != '>')
                {
                    i = i + 1;
                }
                
                while(xml_content[i+1] != '<')
                {
                    operation[j] = xml_content[i+1];
                    j = j + 1;
                    i = i + 1;
                }
            }
        }
        
        i = i + 1;
    }
    
    operation[j] = '\0';
    return relay;
}

/**
 * @breif Reads the status of the gpio's that whether they 
 *        are high or low.
 * @param gpio hold the gpio number to be read.
 * @return status of the requested gpio.
 */
int ReadRelayStatus(int gpio)
{
    FILE *fp;
    char path[10];
    int result;
    char read_command[50];

    sprintf(read_command, "cat /sys/class/gpio/gpio%d/value", gpio);
    fp = popen(read_command, "r");

    if (fp == NULL)
    {
        printf("Failed to run command\n" );
        exit(1);
    }

    while (fgets(path, sizeof(path)-1, fp) != NULL);
    pclose(fp);
    result = atoi(path);

    return result;
}

/**
 * @brief Publishes relay status to the device topic DeviceStatus.
 * @return true if message published successfully, else false.
 */
bool PublishToDeviceTopic()
{
    char content[MAX_SIZE];
    FlowMemoryManager memoryManager = FlowMemoryManager_New();
    FlowDevice loggedInDevice = FlowClient_GetLoggedInDevice(memoryManager);
    FlowID targetDeviceID = FlowDevice_GetDeviceID(loggedInDevice);

    sprintf(content, "%s|%d|/3306/0/5850/1|%d|/3306/0/5850/2", targetDeviceID, ReadRelayStatus(73), ReadRelayStatus(8));

    if (FlowMessaging_PublishToDeviceTopic("DeviceStatus", targetDeviceID, NULL, content, strlen(content), MAX_SIZE))
    {
        LOG(LOG_INFO, "%s", content);
        LOG(LOG_INFO, "Published to DeviceStatus successfully");
        return true;
    }

    FlowMemoryManager_Free(&memoryManager);
    return false;
}

/** 
 * @brief Get the notification from FlowCloud, when user changes
 *        the relay status.
 * @param callbackData is used by FlowCloud.
 * @param eventData contains the data which is published by the user.
 */
void mySubscribeEventCallback(FlowMessagingSubscription callbackData, FlowTopicEvent eventData)
{
    LOG(LOG_DBG, "\r\n\r\n");
    LOG(LOG_DBG, "/********** Received Notification ***********/ \n");

    LOG(LOG_DBG, "event Topic = %s\n", FlowTopicEvent_GetTopic(eventData));
    if (FlowTopicEvent_GetTopicDeviceID(eventData) != NULL)
    {
        LOG(LOG_DBG, "event TopicDeviceId = %s\n", FlowTopicEvent_GetTopicDeviceID(eventData));
    }

    if (FlowTopicEvent_GetTopicUserID(eventData) != NULL)
    {
        LOG(LOG_DBG, "event TopicUserId = %s\n", FlowTopicEvent_GetTopicUserID(eventData));
    }

    LOG(LOG_DBG, "event EventID = %s\n", FlowTopicEvent_GetEventID(eventData));
    FlowDatetime eventTime = FlowTopicEvent_GetEventTime(eventData);

    LOG(LOG_DBG, "event EventTime = %s", ctime(&eventTime));
    FlowDatetime expiryTime = FlowTopicEvent_GetExpiryTime(eventData);

    LOG(LOG_DBG, "event ExpiryTime = %s", ctime(&expiryTime));
    LOG(LOG_DBG, "content type = %s\n", FlowTopicEvent_GetContentType(eventData));
    LOG(LOG_DBG, "content = %s\n", FlowTopicEvent_GetContent(eventData));

    if (FlowTopicEvent_GetFromDeviceID(eventData) != NULL)
    {
    	LOG(LOG_DBG, "fromDevice = %s\n", FlowTopicEvent_GetFromDeviceID(eventData));
    }

    if (FlowTopicEvent_GetFromUserID(eventData) != NULL)
    {
    	LOG(LOG_DBG, "fromUser = %s\n", FlowTopicEvent_GetFromUserID(eventData));
    }

    LOG(LOG_DBG, "/**************** end **********************/\n");

    ChangeRelayState(ParseXML(FlowTopicEvent_GetContent(eventData)));

    PublishToDeviceTopic();
}

/**
 * @brief Subscribes the topic to which the user is publishing.
 * @return true on successful subscription of device topic 
 *         DeviceControl, else false.
 */
bool SubscribeToDeviceTopic()
{
    FlowMessagingSubscription subscription;
    FlowMemoryManager memoryManager = FlowMemoryManager_New();
    FlowDevice loggedInDevice = FlowClient_GetLoggedInDevice(memoryManager);
    FlowID targetDeviceID = FlowDevice_GetDeviceID(loggedInDevice);

    sleep(1);
    subscription = FlowMessaging_SubscribeToDeviceTopic("DeviceControl", targetDeviceID, &mySubscribeEventCallback);

    if (subscription != NULL)
    {
        LOG(LOG_INFO, "\r\nDeviceControl Subscribed Successfully\r\n");
    }
    else
    {
        LOG(LOG_ERR, "\r\nSubscription Failed\r\n ");
        LOG(LOG_ERR, "ERROR: code %d\r\n", FlowThread_GetLastError());
        return false;
    }
    return true;
}

/**
 * @brief Checks whether flow access object is registered or not,
 *        which shows the provisioning status of device.
 * @param *session holds client session.
 * @return true if successfully provisioned, else false.
 */
static bool WaitForGatewayProvisioning(AwaClientSession *session)
{
    bool success = false;
    AwaClientGetOperation *operation = AwaClientGetOperation_New(session);
    char instancePath[URL_PATH_SIZE] = {0};
    if (operation != NULL)
    {
        if (AwaAPI_MakeObjectInstancePath(instancePath,URL_PATH_SIZE,FLOW_ACCESS_OBJECT_ID, 0) == AwaError_Success)
        {
            if (AwaClientGetOperation_AddPath(operation, instancePath) == AwaError_Success)
            {
                if (AwaClientGetOperation_Perform(operation,OPERATION_TIMEOUT) == AwaError_Success)
                {
                    const AwaClientGetResponse *response = NULL;
                    response = AwaClientGetOperation_GetResponse(operation);
                    if (response)
                    {
                        if (AwaClientGetResponse_ContainsPath(response, instancePath))
                        {
                            LOG(LOG_INFO, "Gateway is provisioned.\n");
                            success = true;
                        }

                    }

                }

            }

        }

        AwaClientGetOperation_Free(&operation);
    }
    return success;
}

/**
 * @brief Create a fresh session with client.
 * @param port client's IPC port number.
 * @param *address ip address of client daemon.
 * @return pointer to client's session.
 */
AwaClientSession *Client_EstablishSession(unsigned int port, const char *address)
{
    AwaClientSession * session;
    session = AwaClientSession_New();

    if (session != NULL)
    {
        if (AwaClientSession_SetIPCAsUDP(session, address, port) == AwaError_Success)
        {
            if (AwaClientSession_Connect(session) != AwaError_Success)
            {
                LOG(LOG_ERR, "AwaClientSession_Connect() failed\n");
                AwaClientSession_Free(&session);
            }
        }
        else
        {
            LOG(LOG_ERR, "AwaClientSession_SetIPCAsUDP() failed\n");
            AwaClientSession_Free(&session);
        }
    }
    else
    {
        LOG(LOG_ERR, "AwaClientSession_New() failed\n");
    }
    return session;
}

/**
 * @brief relay gateway application, on/off the relays placed on ci40 as per the received notification
 *        from the FlowCloud User, which holds information of the relay number to be switched on/off,
 *        application also publishes the relay's status after a specified amount of time. 
 */
int main(int argc, char **argv)
{
    int i=0,ret;
    FILE *configFile;
    int isDeviceRegistered;
    const char *fptr = NULL;
    ret = ParseCommandArgs(argc, argv, &fptr);
    
    if (ret <= 0)
    {
        return ret;
    }
    
    if (fptr)
    {
        configFile = fopen(fptr, "w");
        if (configFile != NULL)
        {
            debugStream  = configFile;
        }
        else
        {
            LOG(LOG_ERR, "Failed to create or open %s file", fptr);
        }
    }

    AwaClientSession *clientSession = NULL;

    LOG(LOG_INFO, "Relay Gateway Application");

    LOG(LOG_INFO, "-------------------------\n");

    clientSession = Client_EstablishSession(IPC_CLIENT_PORT, IP_ADDRESS);
    if (clientSession != NULL)
    {
        LOG(LOG_ERR, "Client session established\n");
    }

    LOG(LOG_INFO, "Wait until Gateway device is provisioned\n");

    while (!WaitForGatewayProvisioning(clientSession))
    {
        LOG(LOG_INFO, "Waiting...\n");
        AwaClientSession_Free(&clientSession);
        sleep(2);
        clientSession = Client_EstablishSession(IPC_CLIENT_PORT, IP_ADDRESS);
    }
    
    for (i = FLOW_SERVER_CONNECT_TRIALS; i > 0; i--)
    {
        isDeviceRegistered = InitializeAndRegisterFlowDevice();
        if (isDeviceRegistered)
        {
            break;
        }
        LOG(LOG_INFO, "Try to connect to Flow Server for %d more trials..\n", i);
        sleep(1);
    }
    
    CheckGpio();

    if(!SubscribeToDeviceTopic())
    {
    	goto below;
    }

    while(1)
    {
        if(!PublishToDeviceTopic())
        {
        	LOG(LOG_INFO, "Error while publishing message");
        }
        sleep(HEARTBEAT);
    }
    
    below:
    /*Application should never come here*/
    if (AwaClientSession_Disconnect(clientSession) != AwaError_Success)
    {
        LOG(LOG_ERR, "Failed to disconnect client session");
    }

    if (AwaClientSession_Free(&clientSession) != AwaError_Success)
    {
        LOG(LOG_WARN, "Failed to free client session");
    }

    LOG(LOG_INFO, "Relay Gateway Application Failure");
    return 0;
}

