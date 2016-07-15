/***************************************************************************************************
 * Copyright (c) 2016, Imagination Technologies Limited and/or its affiliated group companies
 * and/or licensors
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification, are permitted
 * provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of conditions
 *    and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY
 * WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file  relay_gateway.c
 * @brief Relay controller application acts as a AwaLWM2M client and observes the IPSO resource for
          digital output. On receipt of awaLWM2M notification, Ci40 will change the relay state
          according to current IPSO resource state.
 *        .
 */

/***************************************************************************************************
 * Includes
 **************************************************************************************************/

#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <libconfig.h>
#include "awa/static.h"
#include "log.h"

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

/** Calculate size of array. */
#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

//! @cond Doxygen_Suppress
#define IP_ADDRESS                  "127.0.0.1"
#define RELAY_STATE                 "DigitalOutputState"
#define RELAY_OBJECT_NAME           "Relay"
#define RELAY_OBJECT_ID             (3201)
#define RELAY_RESOURCE_ID           (5550)
#define MIN_INSTANCES               (0)
#define MAX_INSTANCES               (1)
#define OPERATION_TIMEOUT           (5000)
#define URL_PATH_SIZE               (16)
#define RELAY_GPIO_PIN              "73"
#define CLIENT_NAME                 "RelayDevice"
#define CLIENT_COAP_PORT            (6001)
#define DEFAULT_PATH_CONFIG_FILE    "/etc/config/relay_gateway.cfg"

//! @endcond

static AwaResult RelayStateResourceHandler(client,
                                              operation,
                                              objectID,
                                              objectInstanceID,
                                              resourceID,
                                              resourceInstanceID,
                                              dataPointer,
                                              dataSize,
                                              changed);

/***************************************************************************************************
 * Typedef
 **************************************************************************************************/

/**
 * A structure to contain resource information.
 */
typedef struct
{
    /*@{*/
    AwaResourceID id; /**< resource ID */
    AwaResourceInstanceID instanceID; /**< resource instance ID */
    AwaResourceType type; /**< type of resource e.g. bool, string, integer etc. */
    const char *name; /**< resource name */
    bool isMandatory; /**< whethe this is mandatory resource or not. */
    AwaResourceOperations operation; /**< Operation types that can be performed on that reosurce. */
    AwaStaticClientHandler handler;
    /*@}*/
} Resource;

/**
 * A structure to contain objects information.
 */
typedef struct
{
    /*@{*/
    char *clientID; /**< client ID */
    AwaObjectID id; /**< object ID */
    AwaObjectInstanceID instanceID; /**< object instance ID */
    const char *name; /**< object name */
    unsigned int numResources; /**< number of resource under this object */
    Resource *resources; /**< resource information */
    /*@}*/
} Object;



/***************************************************************************************************
 * Globals
 **************************************************************************************************/

/** Set default debug level to info. */
int g_debugLevel = LOG_INFO;
/** Set default debug stream to NULL. */
FILE * g_debugStream = NULL;
/** Determines whether we should keep main loop running. */
static volatile int g_keepRunning = 1;
/** Keeps current relay state. */
bool g_relayState = false;
/** Keeps certificate */
char *g_cert = NULL;
/** Keeps bootstrap server url */
const char *g_bootstrapServerUrl = NULL;
/** Keeps path to certificate file */
char *g_certFilePath = NULL;

config_t cfg;

/** Initializing objects. */
static Object objects[] =
{
    {
        CLIENT_NAME,
        RELAY_OBJECT_ID,
        0,
        RELAY_OBJECT_NAME,
        1,
        (Resource []){

                            RELAY_RESOURCE_ID,
                            0,
                            AwaResourceType_Boolean,
                            RELAY_STATE,
                            true,
                            AwaResourceOperations_ReadWrite,
                            RelayStateResourceHandler,
                       },
    }
};

/***************************************************************************************************
 * Implementation
 **************************************************************************************************/

/**
 * Reads current value of specified digital output and stores it on value.
 * @param *value stores gpio value
 * @param pin number to read
 * Returns 0 upon succesfull read, -1 otherwise.
 */
static int ReadGPIO(bool * value, int pin)
{
    const int VALUE_MAX = 30;
    char path[VALUE_MAX];
    char valueStr[3];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
    fd = open(path, O_RDONLY);
    if (-1 == fd) {
        LOG(LOG_ERR, "Failed to open gpio value for reading!\n");
        return(-1);
    }

    if (-1 == read(fd, valueStr, 3)) {
        LOG(LOG_ERR, "Failed to read value!\n");
        return(-1);
    }
    close(fd);
    *value = (bool)!!atoi(valueStr);
    return 0;
}

/**
 * Gets called whenever any operation on resource /3201/0/5550 is requested
 */
static AwaResult RelayStateResourceHandler(AwaStaticClient * client,
                                               AwaOperation operation,
                                               AwaObjectID objectID,
                                               AwaObjectInstanceID objectInstanceID,
                                               AwaResourceID resourceID,
                                               AwaResourceInstanceID resourceInstanceID,
                                               void ** dataPointer,
                                               size_t * dataSize,
                                               bool * changed)
 {
     bool newState = false;
     switch (operation)
     {
         case AwaOperation_CreateObjectInstance:
             return AwaResult_SuccessCreated;

         case AwaOperation_CreateResource:
             return AwaResult_SuccessCreated;

         case AwaOperation_Read:
             if (ReadGPIO(&g_relayState, atoi(RELAY_GPIO_PIN)) == 0)
             {
                 *dataPointer = &g_relayState;
                 *dataSize = sizeof(g_relayState);
                 return AwaResult_SuccessContent;
             }
             return AwaResult_InternalError;

         case AwaOperation_Write:
             newState = **(bool**)dataPointer;
             if (newState == g_relayState)
             {
                 return AwaResult_SuccessChanged;
             }
             g_relayState = **(bool**)dataPointer;
             ChangeRelayState(g_relayState);
             *changed = true;
             return AwaResult_SuccessChanged;

     }
 }

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
 * @bried Reds certificate from given file path
 * @param *filePath path to file containing certificate
 * @param **certificate will hold read certificate
 * @return true on successfull read, false otherwise
 */
bool ReadCertificate(const char *filePath, char **certificate) {
    char *fileContents;
    size_t inputFileSize;
    FILE *inputFile = fopen(filePath, "rb");
    if (inputFile == NULL)
    {
        LOG(LOG_DBG, "Unable to open certificate file under: %s", filePath);
        return false;
    }
    if (fseek(inputFile, 0, SEEK_END) != 0)
    {
        LOG(LOG_DBG, "Can't set file offset.");
        return false;
    }
    inputFileSize = ftell(inputFile);
    rewind(inputFile);
    *certificate = malloc(inputFileSize * (sizeof(char)));
    fread(*certificate, sizeof(char), inputFileSize, inputFile);
    if (fclose(inputFile) == EOF)
    {
        LOG(LOG_WARN, "Couldn't close cerificate file.");
    }
    return true;
}


/**
 * @brief Reads config file and save properties into global variables
 * @param *filePath path to config file
 * @return true on succesfull readm false otherwise
 */
bool ReadConfigFile(const char *filePath) {


    config_init(&cfg);
    if(! config_read_file(&cfg, filePath))
    {
        LOG(LOG_ERR, "Failed to open config file at path : %s", filePath);
        return false;
    }
    if(!config_lookup_string(&cfg, "BOOTSTRAP_URL", &g_bootstrapServerUrl))
    {
        LOG(LOG_ERR, "Config file does not contain BOOTSTRAP_URL property.");
        return false;
    }
    if(!config_lookup_string(&cfg, "CERT_FILE_PATH", &g_certFilePath))
    {
        LOG(LOG_ERR, "Config file does not contain CERT_FILE_PATH property.");
        return false;
    }

    return true;
}

/**
 * @brief Parses command line arguments passed to temperature_gateway_appd.
 * @return -1 in case of failure, 0 for printing help and exit, and 1 for success.
 */
static int ParseCommandArgs(int argc, char *argv[], const char **fptr)
{
    int opt, tmp;
    bool isConfigFileSpecified = false;
    opterr = 0;
    char *configFilePath = NULL;

    while (1)
    {
    	opt = getopt(argc, argv, "l:v:c:");
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
                    g_debugLevel = tmp;
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

            case 'c':
                configFilePath = malloc(strlen(optarg));
                sprintf(configFilePath, "%s", optarg);
                break;

            default:
                PrintUsage(argv[0]);
                return -1;
        }
    }
    if (configFilePath == NULL) {
        configFilePath = malloc(strlen(DEFAULT_PATH_CONFIG_FILE));
        sprintf(configFilePath, "%s", DEFAULT_PATH_CONFIG_FILE);
    }

    if (ReadConfigFile(configFilePath) == false)
    {
        return -1;
    }

    if (configFilePath != NULL)
    {
        free(configFilePath);
    }
    return 1;
}

/**
 * @brief Handles Ctrl+C signal. Helps exit app gracefully.
 */
static void CtrlCHandler(int signal) {
    LOG(LOG_INFO, "Exit triggered...");
    g_keepRunning = 0;
}


/**
 * @brief Turn on or off relay on click board depending on specified state.
 * @param state to be set on relay
 */
void ChangeRelayState(bool state)
{
    system("echo out > /sys/class/gpio/gpio73/direction");
    if(state)
        system("echo 1 > /sys/class/gpio/gpio73/value");
    else
        system("echo 0 > /sys/class/gpio/gpio73/value");

    LOG(LOG_INFO, "Changed relay state on Ci40 board to %d", state);
}


/**
 * @brief Add all resource definitions that belong to object.
 * @param *object whose resources are to be defined.
 * @return pointer to flow object definition.
 */
static bool AddResourceDefinitions(AwaStaticClient *client, Object *object)
{
    int i = 0, error = 1;
    bool success = true;

    /*define resources*/
     for (i = 0; i < object->numResources; i++)
     {
         /* For this application only boolean resources are handled */
         if (object->resources[i].type == AwaResourceType_Boolean)
         {
             if ((error = AwaStaticClient_DefineResource(client,
                                            object->id,
                                            object->resources[i].id,
                                            object->resources[i].name,
                                            object->resources[i].type,
                                            1,
                                            1,
                                            object->resources[i].operation)) != AwaError_Success)
             {
                 LOG(LOG_ERR,
                     "Could not add resource definition (%s [%d]) to object definition. Error : %d",
                     object->resources[i].name,
                     object->resources[i].id,
                     error);
                success = false;
                break;
             }
         } else
         {
             LOG(LOG_WARN, "Unsupported resource type");
         }
     }
    return success;
}


/**
 * @brief Define all objects and its resources with client deamon.
 * @param *session holds client session.
 * @return true if all objects are successfully defined, false otherwise.
 */
static bool DefineClientObjects(AwaStaticClient *client)
{
    unsigned int i;
    bool success = true;

    if (client == NULL)
    {
        LOG(LOG_ERR, "Null parameter passsed to %s()", __func__);
        return false;
    }

    for (i = 0; (i < ARRAY_SIZE(objects)) && success; i++)
    {

        AwaStaticClient_DefineObject(client, objects[i].id, objects[i].name, MIN_INSTANCES, MAX_INSTANCES);
        if (AddResourceDefinitions(client, &objects[i]) == false)
        {
            LOG(LOG_ERR, "Failed to add resource definitions.");
            success = false;
            break;
        }
    }
    return success;
}

/**
 * @brief Create object instances on client daemon.
 * @param *session holds client session.
 * @return true if objects are successfully defined on client, false otherwise.
 */
static bool CreateObjectInstances(AwaStaticClient *client)
{
    bool success = true;
    if (client == NULL)
    {
        LOG(LOG_ERR, "Null parameter passsed to %s()", __func__);
        return false;
    }

    int i;
    int error = 0;

    for (i = 0; (i < ARRAY_SIZE(objects)); i++)
    {
        if ((error = AwaStaticClient_CreateObjectInstance(client, objects[i].id, objects[i].instanceID)) != AwaError_Success)
        {
            LOG(LOG_ERR, "Failed to create instance %d of object %d. Error: %d", objects[i].instanceID, objects[i].id, error);
            success = false;
            break;
        }
    }

    return success;
}



/*
 * This function sets resource operation handlers.
 * @param *session holds client session
 * @return true when all handlers are set properly, false otherwise.
 */
static bool SetResourceOperationHandlers(AwaStaticClient *client)
{
    char path[URL_PATH_SIZE] = {0};
    int i,j;
    bool success = true;

    for (i = 0; (i < ARRAY_SIZE(objects)); i++) {
        for (j = 0; (j < objects[i].numResources); j++)
        {

            if (AwaStaticClient_SetResourceOperationHandler(client,
                                                            objects[i].id,
                                                            objects[i].resources[j].id,
                                                            objects[i].resources[j].handler) != AwaError_Success)
            {
                LOG(LOG_ERR, "Failed to set resource operation handler.");
                success = false;
                break;
            }
        }
        if (success == false)
        {
            // In case of failure don't bother trying to make other subscriptions. We will exit app anyway.
            break;
        }
    }
    return success;
}

static AwaStaticClient *PrepareStaticCLient()
{
    AwaStaticClient * awaClient = AwaStaticClient_New();

    if (awaClient == NULL)
    {
        LOG(LOG_ERR, "Failed to create new AWA static client.");
        return NULL;
    }

    AwaStaticClient_SetLogLevel(AwaLogLevel_Warning);
    AwaStaticClient_SetEndPointName(awaClient, CLIENT_NAME);
    AwaStaticClient_SetCoAPListenAddressPort(awaClient, "0.0.0.0", CLIENT_COAP_PORT);
    AwaStaticClient_SetBootstrapServerURI(awaClient, g_bootstrapServerUrl);
    AwaStaticClient_Init(awaClient);
    AwaStaticClient_SetCertificate(awaClient, g_cert, strlen(g_cert), AwaSecurityMode_Certificate);

    return awaClient;
}


/**
 * @brief  Relay gateway application observes the IPSO resource for relay
 *         on client daemon and changes relay state when notification is
 *         received.
 */
int main(int argc, char **argv)
{
    int i=0, ret;
    FILE *configFile;
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
            g_debugStream  = configFile;
        }
        else
        {
            LOG(LOG_ERR, "Failed to create or open %s file", fptr);
        }
    }

    signal(SIGINT, CtrlCHandler);

    LOG(LOG_INFO, "Relay Gateway Application ...");

    LOG(LOG_INFO, "------------------------\n");

    int tmp = 0;
    tmp = system("/usr/bin/export_gpio.sh " RELAY_GPIO_PIN);

    if (tmp != 0)
    {
        LOG(LOG_ERR, "Failed to export GPIO %s for Relay. Exiting...", RELAY_GPIO_PIN);
        g_keepRunning = false;
    }

    LOG(LOG_INFO, "Looking for certificate file under : %s", g_certFilePath);
    while (!ReadCertificate(g_certFilePath, &g_cert))
    {
        sleep(2);
        if (g_keepRunning == false)
        {
            break;
        }
    }

    AwaStaticClient * staticClient = NULL;
    if (g_keepRunning)
    {
        LOG(LOG_INFO, "Certificate found. ");
        staticClient = PrepareStaticCLient();
    }

    if (g_keepRunning && staticClient == NULL)
    {
        LOG(LOG_ERR, "Failed to establish client session. Exiting...");
        g_keepRunning = false;
    }

    if (g_keepRunning && !DefineClientObjects(staticClient))
    {
        LOG(LOG_ERR, "Failed to define client objects. Exiting...");
        g_keepRunning = false;
    }

    if (g_keepRunning && !SetResourceOperationHandlers(staticClient))
    {
        LOG(LOG_ERR, "Failed to subscribe to relay state change. Exiting...");
        g_keepRunning = false;
    }

    if (g_keepRunning && !CreateObjectInstances(staticClient))
    {
        LOG(LOG_ERR, "Failed to create object instances. Exiting...");
        g_keepRunning = false;
    }

    if (g_keepRunning)
    {
        LOG(LOG_INFO, "Observing IPSO object on path /3201/0/5550");
    }


    while (g_keepRunning) {
        AwaStaticClient_Process(staticClient);
        sleep(1);
    }

    if (staticClient != NULL)
    {
        AwaStaticClient_Free(&staticClient);
    }

    if (g_cert != NULL)
    {
        free(g_cert);
    }

    config_destroy(&cfg);

    LOG(LOG_INFO, "Relay Gateway Application Failure");
    return -1;
}
