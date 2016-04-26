#include "cJSON.h"
#include "MQTTClient.h"

#define BUFSIZE		800
#define RBUFSIZE	400
#define kPort		1883
#define MAXTOPIC	125
#define kQOS		QOS1

extern Network gNet;
extern Client gClient;

extern void TCPClt(void *argu);
