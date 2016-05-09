#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "MQTTClient.h"
#include "utask.h"

Network gNet;
Client gClient;
MQTTMessage gMsg;

MQTTPacket_connectData gMData = MQTTPacket_connectData_initializer; 
unsigned char gABuf[BUFSIZE], gRBuf[RBUFSIZE];

char gTopicUp[MAXTOPIC];
char gTopicDown[MAXTOPIC];

void PublishDevice(int source);
char *CreatDL(int upstreamId);

void TCPClt(void *argu)
{
	int sock;
	UNUSED(argu);

	NewNetwork(&gNet);
	while(1){
		sock = ConnectNetwork(&gNet, (char *)kServer, kPort);
		if (sock >= 0 ) break;
		HAL_UART_Transmit_DMA(&huart3, (uint8_t *)"NO connect", 10);
		osDelay(2000);
	} 
	MQTTClient(&gClient, &gNet, 1000, gABuf, BUFSIZE, gRBuf, RBUFSIZE);
	gMData.willFlag = 0;
	gMData.MQTTVersion = 3;
	gMData.clientID.cstring = (char *)kUserid;
	gMData.username.cstring = (char *)kUserid;
	gMData.password.cstring = (char *)kToken;
	gMData.keepAliveInterval = 60;
	gMData.cleansession = 1;		
	sock = MQTTConnect(&gClient, &gMData);
	if (sock	==OKDONE) HAL_UART_Transmit_DMA(&huart3, (uint8_t *)"connect", 7);
	sprintf(gTopicDown, "power/agents/%s/downstream", kUserid);
//	sock = MQTTSubscribe(&gClient, gTopicDown, kQOS, messageArrived);
	PublishDevice(0);
	while(1)
		{
			if(xQueueReceive(xPubQueue, &sock, 1) == pdPASS)
				PublishDevice(sock);
			MQTTYield(&gClient, 1000);
		}
//		
//	MQTTDisconnect(&gClient);
//	gNet.disconnect(&gNet);
}

char *CreatDL(int upstreamId){  //creat device list(upStream)
	cJSON *root, *son1, *son2, *son3;
	char *out;
	
	root = cJSON_CreateArray();
	switch(upstreamId)
	{
		case 0:
		cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
		cJSON_AddStringToObject(son1, "hwid", kUserid);
		cJSON_AddStringToObject(son1, "type", "AGENT");
		cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateArray());	
		cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
		cJSON_AddStringToObject(son3, "key", "interval");
		cJSON_AddNumberToObject(son3, "value", 60);
		break;
		
		case 1:
		cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
		cJSON_AddStringToObject(son1, "hwid", "001");
		cJSON_AddStringToObject(son1, "type", "SENSOR");
		cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateArray());	
		cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
		cJSON_AddStringToObject(son3, "key", "tem");
		cJSON_AddNumberToObject(son3, "value", tem);
		cJSON_AddItemToArray(son2, son3=cJSON_CreateObject());
		cJSON_AddStringToObject(son3, "key", "hum");
		cJSON_AddNumberToObject(son3, "value", hum); 
		break;
	}
	out=cJSON_PrintUnformatted(root);	
	if (out == NULL) {
		HAL_UART_Transmit_DMA(&huart3, (uint8_t *)"null", 4);
		osDelay(0);}
	cJSON_Delete(root);
	return out;
}
//char *CreatValues(int source){}
	
void PublishDevice(int upstreamId)
{
	char *out;
	sprintf(gTopicUp, "power/agents/%s/upstream", kUserid);
	
	gMsg.qos = QOS1;
	gMsg.retained = 0;
	gMsg.dup = 0;
	
	out = gMsg.payload = CreatDL(upstreamId);
	gMsg.payloadlen = strlen(out);
	
	MQTTPublish(&gClient, gTopicUp, &gMsg);
	
	free(out);
}	

void StartPacketsProcess(void * argument)
{
  /* USER CODE BEGIN StartPacketsProcess */
	memset(UART3_RxBuf, 0, strlen(UART3_RxBuf));
  /* Infinite loop */
  for(;;)
  {
    xSemaphoreTake(recFlagHandle, portMAX_DELAY ); 
		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8);
		UART3_RxBuf[UART3_RxBuf_Index] = 0;
		cJSON * msg = cJSON_Parse(UART3_RxBuf);
		cJSON *fnId = cJSON_GetObjectItem(msg,"fnId");
		cJSON *type = cJSON_GetObjectItem(msg,"type");
		cJSON *hwid = cJSON_GetObjectItem(msg,"hwid");
	if (hwid != NULL)
	{
		char 	*hwid_ch = hwid->valuestring;
		HAL_UART_Transmit_DMA(&huart3, (uint8_t *)hwid_ch, strlen(hwid_ch));
	}
		else HAL_UART_Transmit_DMA(&huart3, (uint8_t *)UART3_RxBuf, strlen(UART3_RxBuf));
		osDelay(1);
		cJSON_Delete(msg);
		UART3_RxBuf_Index = 0;
  }
  /* USER CODE END StartPacketsProcess */
}

