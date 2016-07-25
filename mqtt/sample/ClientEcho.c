/*******************************************************************************
 * The J1ST.IO 
 *    http://j1st.io or http://developer.j1st.io
 * 
 * Contributors:
 *    1) Update 1st ClientEcho.
 *******************************************************************************/
/* Standard includes. */
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* J1ST MQTT includes. */
#include "cJSON.h"
#include "MQTTClient.h"

/* J1ST.IO M2M server*/
#define MAXFILENAME 80
#define DEFAULTPORT 1883
#define DEFAULTHOST "developer.j1st.io"
#define DEFAULTAGENT "577a2c956097e90494be7fc7"
#define DEFAULTTOKEN "GejGxXUnfRaITqQOeYtJFHOCcHPwxeGw"

#define BUFSIZE		800
#define RBUFSIZE	400

int gEInterval=300, gFinish=0;
int gPort=DEFAULTPORT;
char gHost[MAXFILENAME+1];
char gAgent[MAXFILENAME+1], gToken[MAXFILENAME+1];
char gTopicUp[MAXFILENAME+1], gTopicDown[MAXFILENAME+1];

xQueueHandle xPubQueue;

extern int jNetSubscribeT(jNet *, const char *, enum QoS, messageHandler);


int publishData(jNet *pJnet, int upstreamId)
{
    int rc;

    cJSON *root, *son1, *son2;
    char *out;

    root = cJSON_CreateArray();
    switch(upstreamId)
    {
        case 0:
        cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
        cJSON_AddStringToObject(son1, "hwid", gAgent);
        cJSON_AddStringToObject(son1, "type", "AGENT");
        cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
        cJSON_AddNumberToObject(son2, "interval", gEInterval);					
        break;
				
        case 1:
        cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
        cJSON_AddStringToObject(son1, "hwid", gAgent);
        cJSON_AddStringToObject(son1, "type", "AGENT");
        cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
        cJSON_AddStringToObject(son2, "testMessage", "Hello J1ST!");
        break;
    }

    out=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    rc = jNetPublishT(pJnet, gTopicUp, out);
    if(rc == 0)
        printf("Published on topic %s: %s, result %d.\n", gTopicUp, out, rc);
    free(out);
    return rc;
}

void messageArrived(MessageData* data)
{
    printf("Message arrived on topic %.*s: %.*s\n", data->topicName->lenstring.len, data->topicName->lenstring.data,
        data->message->payloadlen, data->message->payload);
}

void prvMQTTEchoTask(void *argu)
{
    /* connect to developer.j1st.io, subscribe to a topic, send and receive messages regularly every 1 sec */
    strcpy(gHost, DEFAULTHOST);
    strcpy(gAgent, DEFAULTAGENT);
    strcpy(gToken, DEFAULTTOKEN);
		
    sprintf(gTopicDown, "agents/%s/downstream", gAgent);
    sprintf(gTopicUp, "agents/%s/upstream", gAgent);
	
    int rc;
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
            printf("Cannot connect to :%s, rc: %d. \n", gHost, rc);
            vTaskDelay(1000);
            continue;
        }
        printf("Connect to J1ST.IO server %s:%d succeeded.\n", gHost, gPort);
    
        rc = jNetSubscribeT(pJnet, gTopicDown, QOS2, messageArrived);
        if (rc != 0) goto clean;
        printf("Subscribe the topic of \"%s\" result %d.\n", gTopicDown, rc);
		
        if(publishData(pJnet, 1) != 0) goto clean;
				xQueueSendToBack(xPubQueue, &rc, 0);
        
				do
        {
            /*Demand sending data*/
            short sock;
            if (xQueueReceive(xPubQueue, &sock, 1) == pdPASS)
            {
                printf("Rcvd: xPubQueue %d.\n", sock);
                if (publishData(pJnet, sock)) goto clean;
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

void vStartMQTTTasks(uint16_t usTaskStackSize, UBaseType_t uxTaskPriority)
{
    BaseType_t x = 0L;

    xTaskCreate(prvMQTTEchoTask,    /* The function that implements the task. */
        "MQTTEcho0",    /* Just a text name for the task to aid debugging. */
        usTaskStackSize,    /* The stack size suggestion is 512. */
        (void *)x,    /* The task parameter, not used in this case. */
        uxTaskPriority,    /* The priority assigned to the task suggestion is 3. */
        NULL);    /* The task handle is not used. */
	
    xPubQueue = xQueueCreate(6, 2);
}
/*-----------------------------------------------------------*/
