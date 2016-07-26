#include <string.h>
#include <stdlib.h>
#include <errno.h>			//NOTE: There are some differences between ARM Limited and Linux Limited of <errno.h> 


#include "FreeRTOS.h" 	//NOTE: You may need to modify the import path(rtos)
#include "task.h"				//NOTE: You may need to modify the import path(rtos)

#include "lwip/sockets.h" //NOTE: You may need to modify the import path(lwip)
#include "lwip/netdb.h" //NOTE: LWIP_DNS==1, Turn on DNS module in "lwipopts.h"

#include "j1stio.h"
#include "MQTTClient.h"

char jTmrExpired(jTimer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return left < 0;
}


void jTmrStart(jTimer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout * 1000 / portTICK_RATE_MS; 
}


void jTmrStart_ms(jTimer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout / portTICK_RATE_MS; 
}

int jTmrLeft(jTimer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return (left < 0) ? 0 : left * portTICK_RATE_MS;    
}


void jTmr(jTimer* timer, int id)
{
    timer->end_time = 0;
    timer->t_id = id;
}

char expired(Timer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return left < 0;
}

void countdown_ms(Timer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout / portTICK_RATE_MS; 
}

void countdown(Timer* timer, unsigned int timeout)
{
    timer->end_time = xTaskGetTickCount() + timeout * 1000 / portTICK_RATE_MS; 
}

int left_ms(Timer* timer)
{
    long left = timer->end_time - xTaskGetTickCount();
    return (left < 0) ? 0 : left * portTICK_RATE_MS;    
}


void InitTimer(Timer* timer, int id)
{
    timer->end_time = 0;
    timer->t_id = id;
}


int jnet_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
    if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
    {
        interval.tv_sec = 0;
        interval.tv_usec = 1000;
    }

    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

    int bytes = 0;
    while (bytes < len)
    {
        int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
        if (rc == -1)
        {
						if (errno == EAGAIN) return -2;
						else if (errno == EINTR) continue;
						else return -1;
						//if (errno != ENOTCONN && errno != ECONNRESET)
						//{
						//}
        }
        else if (rc == 0) return -1;
        //else if (rc==0) break;
        else bytes += rc;
    }
    return bytes;
}


int checkWaitingPacket(Network* n, Timer* timer)
{
    int left = left_ms(timer);
    if (left <= 0) return 0;
    
    fd_set readfds;

    FD_ZERO(&readfds);
    FD_SET(n->my_socket, &readfds);

    struct timeval tv= {left / 1000, left % 1000 * 1000};
    if (select(n->my_socket+1, &readfds, NULL, NULL, &tv) > 0) return 1;
    else return 0;
}

int jnet_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
    struct timeval tv;

    tv.tv_sec = 0;  /* 30 Secs Timeout */
    tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

    setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
    int	rc = write(n->my_socket, buffer, len);
    return rc;
}

jNet * jNetInit(void)
{
    jNet *tNet;

    struct Client *pClient;
    struct Network *pNetwork;
    unsigned char *pSendBuf;
    unsigned char *pRcvBuf;

    if ((pRcvBuf = (unsigned char *)malloc(MAX_RCVBUF)) == NULL)	goto err1;
    if ((pSendBuf = (unsigned char *)malloc(MAX_SENDBUF)) == NULL) goto err2;
    if ((pClient = (Client *)malloc(sizeof(struct Client))) == NULL) goto err3;
    if ((pNetwork = (Network *)malloc(sizeof(struct Network))) == NULL) goto err4;
    if ((tNet = (jNet *)malloc(sizeof(struct jNet))) == NULL) goto err5;

    pNetwork->my_socket = 0;
    pNetwork->mqttread = jnet_read;
    pNetwork->mqttwrite = jnet_write;

		tNet->pNet = pNetwork;
    tNet->pClient = pClient;
    tNet->pSendBuf = pSendBuf;
    tNet->pRcvBuf = pRcvBuf;

    // TODO: ?
    return tNet;

err5:
    if (pNetwork)	free(pNetwork);
err4:
    if (pClient)	free(pClient);
err3:
    if (pSendBuf)	free(pSendBuf);
err2:
    if (pRcvBuf)	free(pRcvBuf);
err1:
    return NULL;
}

void jNetFree(jNet *pNet)
{
    free(pNet->pNet);
    free(pNet->pClient);
    free(pNet->pRcvBuf);
    free(pNet->pSendBuf);
    free(pNet);
}

int CreateTCPConnect(char *srv, int port)
{
    int sockfd, error;
    struct sockaddr_in servaddr;
    socklen_t len;
    struct hostent *host;
    
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return sockfd;

		host = gethostbyname(srv);
    if (NULL == host || host->h_addrtype != AF_INET) 
		{
			close(sockfd);
			return -2;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
		memcpy(&servaddr.sin_addr, host->h_addr, sizeof(struct in_addr));
		//inet_aton(srv, &(servaddr.sin_addr));
    //inet_pton(AF_INET, srv, &servaddr.sin_addr);

		// TODO: Use SetSockOpt to
    error = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    if (error == 0)
    {
        error = getsockname(sockfd, (struct sockaddr *)&servaddr, &len);
        //if (error  >= 0) printf("Server %s connected, local port %d\n", srv, ntohs(servaddr.sin_port));
        return sockfd;
    }
    else
    {
				//printf("Error connecting %d\n", error);
        close(sockfd);
		return error;
    }
}

int jNetConnect(jNet *pJnet, const char *host, short nPort,
                const char *agentId, const char *token)
{
    int ret;
		MQTTPacket_connectData gMData = MQTTPacket_connectData_initializer;

    ret = CreateTCPConnect((char *)host, nPort);
    if (ret < 0)
    {
        //printf("No IP address or stack not ready\n");
        return -1;	// No IP address or stack not ready
    }
    pJnet->pNet->my_socket = ret;
    MQTTClient(pJnet->pClient, pJnet->pNet, JTIMEOUT,
               pJnet->pSendBuf, MAX_SENDBUF, pJnet->pRcvBuf, MAX_RCVBUF);
		//printf("Client done!\n");
    // TODO
    gMData.willFlag = 0;
    gMData.MQTTVersion = 4;
    gMData.clientID.cstring = (char *)agentId;
    gMData.username.cstring = (char *)agentId;
    gMData.password.cstring = (char *)token;
    gMData.keepAliveInterval = KEEPALIVEINTERVAL;
    gMData.cleansession = 1;
    ret = MQTTConnect(pJnet->pClient, &gMData);
    if (ret != 0) close(pJnet->pNet->my_socket);
    return ret;
}

int jNetDisconnect(jNet *pJnet)
{
    MQTTDisconnect(pJnet->pClient);
		return close(pJnet->pNet->my_socket);
}

int jNetYield(jNet *pJnet)
{
    return MQTTYield(pJnet->pClient, 1000);
}

int jNetSubscribeT(jNet *pJnet, const char *topic,
				   enum QoS q, messageHandler f)
{
	return MQTTSubscribe(pJnet->pClient, topic, q, f);
}

int jNetPublishT(jNet *pJnet, const char *topic, char *payload)
{
    MQTTMessage msg;
    msg.qos = QOS1;
    msg.retained = 0;
    msg.dup = 0;
    msg.payload = payload;
    msg.payloadlen = strlen(payload);
    return MQTTPublish(pJnet->pClient, topic, &msg);
}


