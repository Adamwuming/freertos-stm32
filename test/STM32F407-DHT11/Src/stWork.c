#include <string.h>
#include <stdlib.h>
#include "cJSON.h"
#include "MQTTClient.h"
#include "utask.h"

#include "user_mb_app.h"
/*Private variables*/
#define MAXFILENAME 79

#define DEFAULTPORT 1883
//#define DEFAULTHOST "developer.j1st.io"
//#define DEFAULTAGENT "577e0de1280c474b40aad807"
//#define DEFAULTTOKEN "DMrgzzxGaqYhMxuIaEiKKDekGqgYLGKU"
#define DEFAULTHOST "139.196.230.150"
#define DEFAULTAGENT "577a2c956097e90494be7fc7"
#define DEFAULTTOKEN "GejGxXUnfRaITqQOeYtJFHOCcHPwxeGw"

extern int jNetSubscribeT(jNet *, const char *, enum QoS, messageHandler);

int gEInterval=300, gFinish=0, gConnect=0;
const char *gTopicUp = "jsonUp";
char gTopicDown[MAXFILENAME+1];


int PublishData(jNet *pJnet, int upstreamId)
{
  int rc=-1, deltaTime;
  cJSON *root, *son1, *son2;
  char *out;
	
  switch(upstreamId){
  case PUB_TYPE_AGENT:
    root = cJSON_CreateArray();
			
    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
    cJSON_AddStringToObject(son1, "type", "AGENT");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
    cJSON_AddNumberToObject(son2, "interval", gEInterval);
						
    out=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    rc = jNetPublishT(pJnet, gTopicUp, out);
    /*Need to match the cJson_free(cJosn.c)*/
    free(out);
//    out = NULL;		
			
    if(rc == 0)
    {
      Print(out);
      Print("\n");
     }
    break;
				
  case PUB_TYPE_DHT:
    root = cJSON_CreateArray();
			
    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
    cJSON_AddStringToObject(son1, "type", "AGENT");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
    cJSON_AddNumberToObject(son2, "Tem", gDHT->pickTem);
    cJSON_AddNumberToObject(son2, "Hem", gDHT->pickHum);
			
    out=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    rc = jNetPublishT(pJnet, gTopicUp, out);
    /*Need to match the cJson_free(cJosn.c)*/
    free(out);
//    out = NULL;
		
    if(rc)
    {
      WriteDHTFlash((uint8_t *)gDHT);
      initDHT();
    }
    else{  
      Print(out);
      Print("\n");
    }
    break;
			
  case PUB_TYPE_HISTORY_DHT:	
    while(!ReadDHTFlash((uint8_t *)gDHT))
    {
      deltaTime = (HAL_GetTick() - gDHT->pickTime)/1000;
      root = cJSON_CreateArray();
					
      cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
      cJSON_AddStringToObject(son1, "type", "AGENT");
      if( deltaTime> gEInterval )
        cJSON_AddNumberToObject(son1, "dtime", deltaTime);
      cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
      cJSON_AddNumberToObject(son2, "Tem", gDHT->pickTem);
      cJSON_AddNumberToObject(son2, "Hem", gDHT->pickHum);
					
      out=cJSON_PrintUnformatted(root);
      cJSON_Delete(root);

      rc = jNetPublishT(pJnet, gTopicUp, out);
      /*Need to match the cJson_free(cJosn.c)*/
      free(out);
//      out = NULL;
				
      if(rc)
      {
        initDHT();
        break;
      }
      else
      {
        Print(out);
        Print("\n");
        modifyAddrOffset(DHT_Flash_Read_Offset_Addr);
      }
    }
    rc=0;
    break;
				
  case PUB_TYPE_INV:
    root = cJSON_CreateArray();
			
    cJSON_AddItemToArray(root, son1=cJSON_CreateObject());	
    cJSON_AddStringToObject(son1, "dsn", "inv0001");
    cJSON_AddStringToObject(son1, "type", "inv");
    cJSON_AddItemToObject(son1, "values", son2=cJSON_CreateObject());	
    cJSON_AddNumberToObject(son2, "vpv1", usMRegHoldBuf[0][0]);
    cJSON_AddNumberToObject(son2, "vpv2", usMRegHoldBuf[0][1]);
    cJSON_AddNumberToObject(son2, "ipv1", usMRegHoldBuf[0][6]);
    cJSON_AddNumberToObject(son2, "ipv2", usMRegHoldBuf[0][7]);
    cJSON_AddNumberToObject(son2, "iar", usMRegHoldBuf[0][72]);
    cJSON_AddNumberToObject(son2, "ibs", usMRegHoldBuf[0][73]);
    cJSON_AddNumberToObject(son2, "ict", usMRegHoldBuf[0][74]);		
    cJSON_AddNumberToObject(son2, "uar", usMRegHoldBuf[0][77]);
    cJSON_AddNumberToObject(son2, "ubs", usMRegHoldBuf[0][78]);
    cJSON_AddNumberToObject(son2, "uct", usMRegHoldBuf[0][79]);
    cJSON_AddNumberToObject(son2, "etoday", (usMRegHoldBuf[0][62]<<16) + usMRegHoldBuf[0][63]);
			
    out=cJSON_PrintUnformatted(root);
    cJSON_Delete(root);

    rc = jNetPublishT(pJnet, gTopicUp, out);
    /*Need to match the cJson_free(cJosn.c)*/
    free(out);
//    out = NULL;
			
    if(rc == 0){
      Print(out);
      Print("\n");
    }
    break;
  }
  return rc;
}

void UpdateInterval(int newInterval)
{
  int sock = PUB_TYPE_AGENT;
  if (newInterval > 0 && newInterval < 3000) 
	{
    gEInterval = newInterval;
    sprintf(gTmp, "UpdateInterval: %d.\n", gEInterval); 
    Print(gTmp);
    xQueueSendToBack(xPubQueue, &sock, 0);
  }
}

void AnaInterval(cJSON *item)
{
  cJSON *value = cJSON_GetObjectItem(item, "sec");
  if (value != NULL && value->type == cJSON_Number)
  {
    int interval  = (int)(value->valuedouble + 0.0000001);
    sprintf(gTmp, "Rcvd: sec %d.\n", interval); 
    Print(gTmp);
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
    //cJSON *sub = cJSON_GetObjectItem(item, "hwid");
    //if (sub != NULL && sub->type == cJSON_String && !strcmp(DEFAULTAGENT, sub->valuestring))
      func(item);        
  }
}

/*Analytical "Fn Code" definitions from developer console(developer.j1st.io)*/
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
  sprintf(gTmp, "Message arrived on topic %.*s: %.*s\n", md->topicName->lenstring.len, 
        md->topicName->lenstring.data, md->message->payloadlen, md->message->payload); 
  Print(gTmp);
  ParseMsg(payload);
}

void vMQTTTask(void *argu)
{
  int rc, delayS=1;
  int sock = PUB_TYPE_HISTORY_DHT;
  UNUSED(argu);

  sprintf(gTopicDown, "agents/%s/downstream", DEFAULTAGENT);
	
  jNet * pJnet = jNetInit();
  if (NULL == pJnet)
  {
    sprintf(gTmp, "Cannot allocate jnet resources."); 
    Print(gTmp);
    return;
  }
	
  while(!gFinish)
  {
    rc = jNetConnect(pJnet, DEFAULTHOST, DEFAULTPORT, DEFAULTAGENT, DEFAULTTOKEN);
    if (rc != 0 )
    {
      /*rc: No IP address or stack not ready = -1, OKDONE = 0 , AGENT_ID & AGENT_TOKEN is authorized = 5*/
      sprintf(gTmp, "Cannot connect to :%s, rc: %d. Waiting for %d seconds and retry.\n", DEFAULTHOST, rc, delayS); 
      Print(gTmp);
      osDelay(delayS*1000);
      delayS *= 2;
      if(delayS > 30) delayS = 30;
      continue;
    }
    delayS = 1;
    gConnect = 1;
    xQueueSendToBack(xPubQueue, &sock, 0);
    sprintf(gTmp, "Connect to J1ST.IO server %s:%d succeeded.\n", DEFAULTHOST, DEFAULTPORT); 
    Print(gTmp);    
    rc = jNetSubscribeT(pJnet, gTopicDown, QOS2, messageArrived);
    if (rc != 0) goto clean;
    sprintf(gTmp, "Subscribe the topic of \"%s\" result %d.\n", gTopicDown, rc); 
    Print(gTmp);  
    if (PublishData(pJnet, 0) != 0) goto clean;
    do
    {
      /*Demand sending data*/
      int sock;
      if (xQueueReceive(xPubQueue, &sock, 1) == pdPASS)
      {
        sprintf(gTmp, "Rcvd: xPubQueue %d.\n", sock);
        Print(gTmp);  							
        if (PublishData(pJnet, sock) != 0) goto clean;
      }
      /* Make jNet library do background tasks, send and receive messages(PING/PONG) regularly every 1 sec */
      rc = jNetYield(pJnet);
      if (rc < 0) break;
    }  while (!gFinish);
    /* Cleanup */
    clean:
      gConnect=0;
      jNetDisconnect(pJnet);
      sprintf(gTmp, "Connection stopped.\n"); 
      Print(gTmp);  		
  }
  jNetFree(pJnet);
}

