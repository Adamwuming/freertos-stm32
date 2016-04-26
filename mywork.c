#include <string.h>
#include <stdlib.h>
#include "utask.h"

#include "cjson.h"
#include "mqttclient.h"

//volatile int gNewData = 1;
extern volatile int LEDInten;
volatile int gPubInt = 300;

xQueueHandle xPubQueue;

Network gNet;
Client gClient;

static const char *kUserid = "56fdd2856097e9123236f054";
static const char *kToken  = "FaZMBsISqMrKyMXeYbpXbGabPGGqyMJZ";
static const char *kProgid = kUserid;
static const char *kServer = "139.198.0.174";
static const char *kDeli   = "\n";
#define BUFSIZE		800
#define RBUFSIZE	400
#define kPort		1883
#define kQOS		QOS1


unsigned char gABuf[BUFSIZE], gRBuf[RBUFSIZE];

#define MAXTOPIC	80
char gTopic[MAXTOPIC];
char gTopicDown[MAXTOPIC];


MQTTPacket_connectData gMData = MQTTPacket_connectData_initializer; 


void messageArrived(MessageData* md)
{
	MQTTMessage* message = md->message;
	char *payload =  (char*)message->payload;
	
	printf("Rcvd: %.*s \n", (int)message->payloadlen, payload);
	
	// DANGER
	payload[(int)message->payloadlen] = 0;
	
	cJSON *json = cJSON_Parse(payload);
	int count = cJSON_GetArraySize(json);
	for (int i=0; i<count; i++)
	{
		cJSON *son1 = cJSON_GetArrayItem(json, i);
		// More work to determine the device type here
		cJSON *son2 = cJSON_GetObjectItem(son1, "values");
		if (son2 != NULL)
		{
			int count1 = cJSON_GetArraySize(son2);
			for (int j=0; j<count1; j++)
			{
				cJSON *son3 = cJSON_GetArrayItem(son2, j);
				cJSON *key = cJSON_GetObjectItem(son3, "key");
				if (key != NULL)
				{
					cJSON *value = cJSON_GetObjectItem(son3, "value");
					if (value != NULL)
					{
						int n;
						if (value->type == cJSON_Number)
							printf("KEY: %s   VALUE: %f\n", key->valuestring, value->valuefloat);
						else
							printf("KEY: %s   VALUE: %s\n", key->valuestring, value->valuestring);
						if (!strcmp(key->valuestring, "intensity")) 
						{
							n = (int) (value->valuefloat + 0.0001f);
							if (n <= 20 && n >= 0) 
							{
								LEDInten = n;
								SetLEDIntensity(LEDInten);
							}
						}
						if (!strcmp(key->valuestring, "interval")) 
						{
							n = (int) (value->valuefloat + 0.0001f);
							if (n > 0) gPubInt = n;
						}
					}
				}
			}
		}
	}
	cJSON_Delete(json);
//	if (opts.showtopics)
//		printf("%.*s\t", md->topicName->lenstring.len, md->topicName->lenstring.data);
//	if (opts.nodelimiter)
//		printf("%.*s", (int)message->payloadlen, (char*)message->payload);
//	else
//		printf("%.*s%s", (int)message->payloadlen, (char*)message->payload, opts.delimiter);
}

volatile int gRestart = 0;

MQTTMessage gMsg;

char *CreateJStr(int source)
{
	cJSON *root, *son1, *son2, *son3;
	char *out;
	
	root = cJSON_CreateArray();
	if (source == 0)
	{
		cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
		cJSON_AddStringToObject(son1, "hwid", "0000AA1234");
		cJSON_AddStringToObject(son1, "type", "AGENT");
	}
	
	cJSON_AddItemToArray(root, son1=cJSON_CreateObject());
	cJSON_AddStringToObject(son1, "hwid", "0000AC15D648");	
	cJSON_AddStringToObject(son1, "type", "SENSOR");
	cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateArray());	
	cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
	cJSON_AddStringToObject(son3, "key", "intensity");
	cJSON_AddNumberToObject(son3, "value", LEDInten);
	cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
	cJSON_AddStringToObject(son3, "key", "temperture");
	cJSON_AddNumberToObject(son3, "value", gTemperature);	
	if (source == 2)
	{
		cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
		cJSON_AddStringToObject(son3, "key", "ABCAlm");
		cJSON_AddNumberToObject(son3, "value", 1);	
	}
	out=cJSON_PrintUnformatted(root);	cJSON_Delete(root);
	printf("%s\n", out);
	return out;
}

void PublishLED(int source)
{
	char *out;
	int res;
	sprintf(gTopic, "power/agents/%s/upstream", kUserid);
	
	gMsg.qos = QOS1;
	gMsg.retained = 0;
	gMsg.dup = 0;
	
	out = gMsg.payload = CreateJStr(source);
	gMsg.payloadlen = strlen(out);
	
	printf("Publishing %d, len %d ... ", source, gMsg.payloadlen);
	
	res = MQTTPublish(&gClient, gTopic, &gMsg);
	printf("Result %d\n", res);
	
	free(out);
	
}

void TCPClt(void *argu)
{
	int sock;
	UNUSED(argu);
	
	//fillOpts();
	NewNetwork(&gNet);
	
	while(1)
	{
		sock = ConnectNetwork(&gNet, (char *)kServer, kPort);
		if (sock < 0) 
		{
			printf("Cannot connect %s: %d, retry\n", kServer, sock);
			osDelay(2000);
			continue;
		}
		MQTTClient(&gClient, &gNet, 1000, gABuf, BUFSIZE, gRBuf, RBUFSIZE);
		gMData.willFlag = 0;
		gMData.MQTTVersion = 3;
		gMData.clientID.cstring = (char *)kProgid;
		gMData.username.cstring = (char *)kUserid;
		gMData.password.cstring = (char *)kToken;
		gMData.keepAliveInterval = 120;
		gMData.cleansession = 1;		
		sock = MQTTConnect(&gClient, &gMData);
		printf("Connected %d\n", sock);

		sprintf(gTopicDown, "power/agents/%s/downstream", kUserid);
		sock = MQTTSubscribe(&gClient, gTopicDown, kQOS, messageArrived);
		printf("Subscribed %d\n", sock);	
		
		PublishLED(0);		// Initial publish
		while(!gRestart)
		{
			if (xQueueReceive(xPubQueue, &sock, 1) == pdPASS)
				PublishLED(sock);
			MQTTYield(&gClient, 1000);	
		}
		printf("Stopping\n");

		MQTTDisconnect(&gClient);
		gNet.disconnect(&gNet);
	}
}

