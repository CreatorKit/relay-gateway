#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/stat.h>
#include <flow/flowmessaging.h>
#include<unistd.h>
#include <libconfig.h>

/***************************************************************************************************
 * Definitions
 **************************************************************************************************/

/** Max array size of registration data elements. */
#define MAX_SIZE (256)
/** Configuration file to get registration data stored by provisioning app. */
#define CONFIG_FILE "/usr/flow_access.cfg"
/** Number of trials for reading configuration file */
#define FILE_READ_TRIALS (5)

/***************************************************************************************************
 * Prototypes
 **************************************************************************************************/
 
bool SubscribeToDeviceTopic(FlowID targetDeviceID);

/***************************************************************************************************
 * typedef
 **************************************************************************************************/

/**
 * A structure to contain flow registration data.
 */
typedef struct
{
	/*@{*/
	char url[MAX_SIZE];
	char key[MAX_SIZE];
	char secret[MAX_SIZE];
	char deviceName[MAX_SIZE];
	char fcap[MAX_SIZE];
	char devicetype[MAX_SIZE];
	char serial[MAX_SIZE];
	char software_version[MAX_SIZE];
	char DevID[MAX_SIZE];
	char user_name[MAX_SIZE];
	char password[MAX_SIZE];
	/*@}*/
}RegistrationData;

RegistrationData regData;


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
			printf("Waiting for config data");
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
		            GetValueForKey(&cfg, regData->DevID, "DeviceID") &&
		            GetValueForKey(&cfg, regData->user_name, "User") &&
		            GetValueForKey(&cfg, regData->password, "Password"))
			{
				success = true;
			}
			else
			{
				printf("Failed to read config data");
			}
			config_destroy(&cfg);
			break;
		}
	}
	if (i == 0)
	{
		printf("Failed to read config file");
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
		FlowCore_RegisterTypes();
		if (FlowMessaging_Initialise())
		{
			printf("\nFlowMessaging Initialized Successfully...\n");
			if (FlowClient_ConnectToServer(url, key, secret, false))
			{
				printf("\nConnected to FlowServer...\n");
				sleep(1);
				return true;
			}
			else
			{
				FlowCore_Shutdown();
				FlowMessaging_Shutdown();
				printf("Failed to connect to server");
			}
		}
		else
		{
			printf("Flow Messaging initialization failed");
		}
	}
	else
	{
		printf("Flow Core initialization failed");
	}
	return false;
}

bool PublishToDeviceTopic(FlowID targetDeviceID)
{
	int r_number;
	char operation[10];
	st :
	printf("\nEnter the relay number(1/2) : ");
	scanf("%d",&r_number);
	if((r_number<1 || r_number>2))
	{
		goto st;
	}
	printf("\nEnter the operation you want to perform(on/off) : ");
	scanf("%s",operation);
	bool result = false;
	char content[250];
	sprintf(content,"<project><relay>%d</relay><operation>%s</operation></project>",r_number,operation);
	if (FlowMessaging_PublishToDeviceTopic("DeviceControl", targetDeviceID, NULL, content, strlen(content), 300))
	{
		result = true;
	}
	return result;
}

bool SelectDevice2Publish()
{
	bool result = false;
	FlowDevice targetDevice;
	FlowMemoryManager memoryManager = FlowMemoryManager_New();
	if (memoryManager)
	{
		FlowUser loggedInUser = FlowClient_GetLoggedInUser(memoryManager);
		/* Retrieve Owned Devices */
		FlowDevices myDevices = FlowUser_RetrieveOwnedDevices(loggedInUser, 20);
		if (myDevices != NULL)
		{
			/* List my Owned Devices */
			int index = 0;
			int deviceIndex;
			printf("\r\n-- List of user's owned devices --");
			for (index = 0; index < FlowDevices_GetCount(myDevices); index++)
			{
				FlowDevice thisDevice = FlowDevices_GetItem(myDevices, index);
				printf("\r\n %d) Device name: %s", index + 1, FlowDevice_GetDeviceName(thisDevice));
			}
			printf("\r\n\r\nChoose a device index to publish to as displayed in the above list (1-%d): ", FlowDevices_GetCount(myDevices));
			scanf("%d", &deviceIndex);
			while (deviceIndex <= 0 || deviceIndex > FlowDevices_GetCount(myDevices))
			{
				printf("Invalid selection - Try Again..");
				scanf("%d", &deviceIndex);
			}
			getchar();						// Consume left-over carriage return
			deviceIndex--;
			targetDevice = FlowDevices_GetItem(myDevices, deviceIndex);
			FlowID targetDeviceID = FlowDevice_GetDeviceID(targetDevice);
			if (PublishToDeviceTopic(targetDeviceID))
			{
				result = true;
			}
			else
			{
				printf("Send Publish Failed.\r\n");
				printf("ERROR: code %d", FlowThread_GetLastError());
			}
		}
		else
		{
			printf("No devices registered to the logged in user.\r\n");
			printf("ERROR: code %d", FlowThread_GetLastError());
		}

		/*Clearing Memory*/
		FlowMemoryManager_Free(&memoryManager);
	}
	else
	{
		printf("Context is NULL");
	}
	return result;
}

/* @brief  SelectDevice2Subscribe() subscribes the target device 
	   which is publishing 
 */
bool SelectDevice2Subscribe()
{
	bool result = false;
	FlowDevice targetDevice;
	FlowMemoryManager memoryManager = FlowMemoryManager_New();
	if (memoryManager)
	{
		FlowUser loggedInUser = FlowClient_GetLoggedInUser(memoryManager);
		/* Retrieve Owned Devices */
		FlowDevices myDevices = FlowUser_RetrieveOwnedDevices(loggedInUser, 20);
		if (myDevices != NULL)
		{
			/* List my Owned Devices */
			int index = 0;
			int deviceIndex;
			printf("\r\n-- List of user's owned devices --");
			for (index = 0; index < FlowDevices_GetCount(myDevices); index++)
			{
				FlowDevice thisDevice = FlowDevices_GetItem(myDevices, index);
				printf("\r\n %d) Device name: %s", index + 1, FlowDevice_GetDeviceName(thisDevice));
			}
			printf("\r\n\r\nChoose a device index to subscribe to as displayed in the above list (1-%d): ", FlowDevices_GetCount(myDevices));
			scanf("%d", &deviceIndex);
			while (deviceIndex <= 0 || deviceIndex > FlowDevices_GetCount(myDevices))
			{
				printf("Invalid selection - Try Again..");
				scanf("%d", &deviceIndex);
			}
			getchar();						// Consume left-over carriage return
			deviceIndex--;
			targetDevice = FlowDevices_GetItem(myDevices, deviceIndex);
			FlowID targetDeviceID = FlowDevice_GetDeviceID(targetDevice);
			if (SubscribeToDeviceTopic(targetDeviceID))
			{
				result = true;
			}
			else
			{
				printf("Send Subscribe Failed.\r\n");
				printf("ERROR: code %d", FlowThread_GetLastError());
			}
		}
		else
		{
			printf("No devices registered to the logged in user.\r\n");
			printf("ERROR: code %d", FlowThread_GetLastError());
		}

		/*Clearing Memory*/
		FlowMemoryManager_Free(&memoryManager);
	}
	else
	{
		printf("Context is NULL");
	}
	return result;
}

/* @brief subscription callback to get the notification
 * @param callbackData
 * @param eventData
 */
void mySubscribeEventCallback(FlowMessagingSubscription callbackData, FlowTopicEvent eventData)
{
	printf("\r\n\r\n");
	printf("/********** Received Notification ***********/ \n");
	printf("event Topic = %s\n", FlowTopicEvent_GetTopic(eventData));
	if (FlowTopicEvent_GetTopicDeviceID(eventData) != NULL)
		printf("event TopicDeviceId = %s\n", FlowTopicEvent_GetTopicDeviceID(eventData));
	if (FlowTopicEvent_GetTopicUserID(eventData) != NULL)
		printf("event TopicUserId = %s\n", FlowTopicEvent_GetTopicUserID(eventData));
	printf("event EventID = %s\n", FlowTopicEvent_GetEventID(eventData));
	FlowDatetime eventTime = FlowTopicEvent_GetEventTime(eventData);
	printf("event EventTime = %s", ctime(&eventTime));
	FlowDatetime expiryTime = FlowTopicEvent_GetExpiryTime(eventData);
	printf("event ExpiryTime = %s", ctime(&expiryTime));
	printf("content type = %s\n", FlowTopicEvent_GetContentType(eventData));
	printf("content = %s\n", FlowTopicEvent_GetContent(eventData));
	if (FlowTopicEvent_GetFromDeviceID(eventData) != NULL)
		printf("fromDevice = %s\n", FlowTopicEvent_GetFromDeviceID(eventData));
	if (FlowTopicEvent_GetFromUserID(eventData) != NULL)
		printf("fromUser = %s\n", FlowTopicEvent_GetFromUserID(eventData));
	printf("/**************** end **********************/\n");
}

/*@brief subscribes the topic to which device is publishing
 * @param targetDeviceID hold the deviceID of the subscribed device
 */
bool SubscribeToDeviceTopic(FlowID targetDeviceID)
{
	FlowMessagingSubscription subscription;
	bool result = false;
	sleep(1);
	subscription = FlowMessaging_SubscribeToDeviceTopic("DeviceStatus", targetDeviceID, &mySubscribeEventCallback);
	if (subscription != NULL)
	{
		printf("\n\rSubscribe Successful\n\r");
		result = true;
	}
	else
	{
		printf("\n\rSubscription Failed\n\r ");
		printf("ERROR: code %d\n\r", FlowThread_GetLastError());
	}
	return result;
}
int main()
{
	int result = -1;
	int ch=0;
	printf("\n\nRelay Gateway Test Application\n\n");
	printf("------------------------------------------\n\n");
	if (GetConfigData(&regData))
	{
		if (InitialiseLibFlow(regData.url, regData.key, regData.secret))
		{
			if(FlowClient_LoginAsUser(regData.user_name,regData.password, true))
			{
				SelectDevice2Subscribe();
				while(ch <= 2)
				{
					printf("\n1. Change the Device setting and publish to device topic\n\n");
					printf("2. Exit\n\n");
					scanf("%d",&ch);
					if(ch==1)
					{
							if(SelectDevice2Publish())
							{
								result  = 0;
								printf("\r\nPublish Event Successful\r\n");
							}
					}
					if(ch==2)
					{
						printf("Thank You\n\n");
						result=0;
						sleep(1);
						break;
					}
				}
			}
		}
	}
	return result;
}


