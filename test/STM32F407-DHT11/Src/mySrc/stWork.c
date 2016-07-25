#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "MQTTClient.h"
#include "utask.h"

/*mqtt client*/
#define BUFSIZE		800
#define RBUFSIZE	400

/*Private variables*/
#define MAXFILENAME 80

#define DEFAULTPORT 1883
//#define DEFAULTHOST "developer.j1st.io"
//#define DEFAULTAGENT "577f0d1c280c474b40aad873"
//#define DEFAULTTOKEN "pvebPgCRkDUDwSWGkOfVVfprigjvMsyK"
#define DEFAULTHOST "139.198.0.174"
#define DEFAULTAGENT "577a2c956097e90494be7fc7"
#define DEFAULTTOKEN "GejGxXUnfRaITqQOeYtJFHOCcHPwxeGw"

extern int jNetSubscribeT(jNet *, const char *, enum QoS, messageHandler);

int gEInterval=300, gFinish=0, dtimeTickstart=0;
int gPort=DEFAULTPORT;
char gHost[MAXFILENAME+1];
char gAgent[MAXFILENAME+1], gToken[MAXFILENAME+1];
char gTopicUp[MAXFILENAME+1], gTopicDown[MAXFILENAME+1];

int PublishData(jNet *pJnet, int upstreamId)
{
    int rc=-1, i, deltaTime;
		struct DHT11 *p;
		cJSON *root, *son1, *son2;
		char *out;
		
		switch(upstreamId){
		case 0:
		root = cJSON_CreateArray();
		
		cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
		cJSON_AddStringToObject(son1, "hwid", gAgent);
		cJSON_AddStringToObject(son1, "type", "AGENT");
		cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
		cJSON_AddNumberToObject(son2, "interval", gEInterval);
		
		out=cJSON_PrintUnformatted(root);
		cJSON_Delete(root);

		rc = jNetPublishT(pJnet, gTopicUp, out);
		if(rc == 0)
		printf("Published on topic %s: %s, result %d.\n", gTopicUp, out, rc);
		free(out);
		break;
				
		case 1:		
		for(i=0;i<100;i++){
			p=gDHT+i;
			if((*p).pickTime==0) break;
			else{
				deltaTime = (HAL_GetTick() - gDHT[i].pickTime)/1000;
				printf("Publishing i: %d\n", i);

				root = cJSON_CreateArray();
				
				cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
				cJSON_AddStringToObject(son1, "hwid", gAgent);
				cJSON_AddStringToObject(son1, "type", "AGENT");
				if( deltaTime> gEInterval )
					cJSON_AddNumberToObject(son1, "dtime", deltaTime);
				cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
				cJSON_AddNumberToObject(son2, "Tem", gDHT[i].pickTem);
				cJSON_AddNumberToObject(son2, "Hem", gDHT[i].pickHum);
				
				out=cJSON_PrintUnformatted(root);
				cJSON_Delete(root);

				rc = jNetPublishT(pJnet, gTopicUp, out);
				if(rc == 0){
					initDHT(i);
					printf("Published on topic %s: %s, result %d.\n", gTopicUp, out, rc);
				}
				free(out);
			}
		}
		break;

		}
	return rc;
}


void SetParas(void)
{
    strcpy(gHost, DEFAULTHOST);
    strcpy(gAgent, DEFAULTAGENT);
    strcpy(gToken, DEFAULTTOKEN);
		
        
    sprintf(gTopicDown, "agents/%s/downstream", gAgent);
    sprintf(gTopicUp, "agents/%s/upstream", gAgent);
}

void UpdateInterval(int newInterval)
{
		short sock=0;
    if (newInterval > 0 && newInterval < 3000) {
        gEInterval = newInterval;
			  printf("UpdateInterval: %d.\n", gEInterval);
				xQueueSendToBack(xPubQueue, &sock, 0);
    }
}

void AnaInterval(cJSON *item)
{
    cJSON *value = cJSON_GetObjectItem(item, "sec");
    if (value != NULL && value->type == cJSON_Number)
    {
        int interval  = (int)(value->valuedouble + 0.0000001);
        printf("Rcvd: sec %d.\n", interval);
        UpdateInterval(interval);
    }
}


void CheckCmd(cJSON *root, const char *key, void (*func)(cJSON *))
{
    cJSON *item;

    cJSON * cmdArray = cJSON_GetObjectItem(root, key);
    if (cmdArray == NULL) return;
        
    cJSON_ArrayForEach(item, cmdArray)
    {
        cJSON *sub = cJSON_GetObjectItem(item, "hwid");
        if (sub != NULL && sub->type == cJSON_String &&
                !strcmp(gAgent, sub->valuestring))
            func(item);        
    }
}

/*Analytical "Fn Code" definitions from developer console*/
void ParseMsg(char *payload)
{
    cJSON * root = cJSON_Parse(payload);
    if (!root) return;

    CheckCmd(root, "SetInterval", AnaInterval);

    if (root) cJSON_Delete(root);
}

void messageArrived(MessageData* md)
{
    MQTTMessage* message = md->message;
    char *payload =  (char*)message->payload;

    // TODO: Safer
    payload[(int)message->payloadlen] = 0;
    printf("Message arrived on topic %.*s: %.*s\n", md->topicName->lenstring.len, md->topicName->lenstring.data, md->message->payloadlen, md->message->payload);

    ParseMsg(payload);
}

void MQTTWork(void *argu)
{
    int rc, delayS=1;
    UNUSED(argu);

    SetParas();
	
    jNet * pJnet = jNetInit();
    if (NULL == pJnet)
    {
        printf("Cannot allocate jnet resources.");
        return;
    }
	
    while(!gFinish)
    {
        rc = jNetConnect(pJnet, gHost, gPort, gAgent, gToken);
        if (rc != 0 ) 
				{
					/*rc: No IP address or stack not ready = -1, OKDONE = 0 , AGENT_ID & AGENT_TOKEN is authorized = 5*/
					printf("Cannot connect to :%s, sock: %d. Waiting for %d seconds and retry.\n", gHost, rc, delayS);
					osDelay(delayS*1000);
					delayS *= 2;
					if(delayS > 30) delayS = 30;
					continue;
				}
        delayS = 1;
        printf("Connect to J1ST.IO server %s:%d succeeded.\n", gHost, gPort);
    
        rc = jNetSubscribeT(pJnet, gTopicDown, QOS2, messageArrived);
        if (rc != 0) goto clean;
        printf("Subscribe the topic of \"%s\" result %d.\n", gTopicDown, rc);

				if (PublishData(pJnet, 0) != 0) goto clean;
        do
        {
            /*Demand sending data*/
            short sock;
            if (xQueueReceive(xPubQueue, &sock, 1) == pdPASS)
            {
                printf("Rcvd: xPubQueue %d.\n", sock);
                if (PublishData(pJnet, sock) != 0) goto clean;
            }
            /* Make jNet library do background tasks, send and receive messages(PING/PONG) regularly every 1 sec */
            rc = jNetYield(pJnet);
            if (rc < 0) break;
        }  while (!gFinish);
        printf("Stopping...\n");
        /* Cleanup */
clean:
        jNetDisconnect(pJnet);
    }
    jNetFree(pJnet);        
}

//void StartPacketsProcess(void * argument)
//{
//  /* USER CODE BEGIN StartPacketsProcess */
//	memset(UART3_RxBuf, 0, strlen(UART3_RxBuf));
//  /* Infinite loop */
//  for(;;)
//  {
//    xSemaphoreTake(recFlagHandle, portMAX_DELAY ); 
//		HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8);
//		UART3_RxBuf[UART3_RxBuf_Index] = 0;
//		cJSON * msg = cJSON_Parse(UART3_RxBuf);
//		cJSON *fnId = cJSON_GetObjectItem(msg,"fnId");
//		cJSON *type = cJSON_GetObjectItem(msg,"type");
//		cJSON *hwid = cJSON_GetObjectItem(msg,"hwid");
//	if (hwid != NULL)
//	{
//		char 	*hwid_ch = hwid->valuestring;
//		HAL_UART_Transmit_DMA(&huart3, (uint8_t *)hwid_ch, strlen(hwid_ch));
//	}
//		else HAL_UART_Transmit_DMA(&huart3, (uint8_t *)UART3_RxBuf, strlen(UART3_RxBuf));
//		osDelay(1);
//		cJSON_Delete(msg);
//		UART3_RxBuf_Index = 0;
//  }
//  /* USER CODE END StartPacketsProcess */
//}

