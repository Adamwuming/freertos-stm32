#ifndef __MQTT_LWIP_
#define __MQTT_LWIP_

//#include "simplelink.h"
//#include "netapp.h"
#include "socket.h"
//#include "hw_types.h"
//#include "systick.h"

typedef struct Timer Timer;

struct Timer {
	unsigned long systick_period;
	unsigned long end_time;
};

typedef struct Network Network;

struct Network
{
	int my_socket;
	int (*mqttread) (Network*, unsigned char*, int, int);
	int (*mqttwrite) (Network*, unsigned char*, int, int);
	void (*disconnect) (Network*);
};

char expired(Timer*);
void countdown_ms(Timer*, unsigned int);
void countdown(Timer*, unsigned int);
int left_ms(Timer*);

void InitTimer(Timer*);

int LWIP_read(Network*, unsigned char*, int, int);
int LWIP_write(Network*, unsigned char*, int, int);
void LWIP_disconnect(Network*);
void NewNetwork(Network*);

int ConnectNetwork(Network*, char*, int);
//int TLSConnectNetwork(Network*, char*, int, SlSockSecureFiles_t*, unsigned char, unsigned int, char);

#endif
