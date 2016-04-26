#include "MQTTLwIP.h"
#include <string.h>

char expired(Timer* timer) {
	long left = timer->end_time - HAL_GetTick();
	return (left < 0);
}


void countdown_ms(Timer* timer, unsigned int timeout) {
	timer->end_time = HAL_GetTick() + timeout;
}


void countdown(Timer* timer, unsigned int timeout) {
	timer->end_time = HAL_GetTick() + (timeout * 1000);
}


int left_ms(Timer* timer) {
	long left = timer->end_time - HAL_GetTick();
	return (left < 0) ? 0 : left;
}


void InitTimer(Timer* timer) {
	timer->end_time = 0;
}

int LWIP_read(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval interval = {timeout_ms / 1000, (timeout_ms % 1000) * 1000};
	if (interval.tv_sec < 0 || (interval.tv_sec == 0 && interval.tv_usec <= 0))
	{
		interval.tv_sec = 0;
		interval.tv_usec = 100;
	}

	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

	int bytes = 0;
	while (bytes < len)
	{
		int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
		if (rc == -1)
		{
//			if (errno != ENOTCONN && errno != ECONNRESET)
//			{
				bytes = -1;
				break;
//			}
		}
		else if (rc == 0) break;
		else bytes += rc;
	}
	return bytes;
}

int checkWaitingPacket(Network* n)
{

	fd_set readfds;
	
	FD_ZERO(&readfds);
	FD_SET(n->my_socket, &readfds);
	
	struct timeval tv= {0, 1000};
	if (select(n->my_socket+1, &readfds, NULL, NULL, &tv) > 0) return 1;
	else return 0;
}

//	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&interval, sizeof(struct timeval));

//	int bytes = 0;
//	while (bytes < len)
//	{
//		int rc = recv(n->my_socket, &buffer[bytes], (size_t)(len - bytes), 0);
//		if (rc == -1)
//		{
////			if (errno != ENOTCONN && errno != ECONNRESET)
////			{
//				bytes = -1;
//				break;
////			}
//		}
//		else
//			bytes += rc;
//	}
//	return bytes;
//}


int LWIP_write(Network* n, unsigned char* buffer, int len, int timeout_ms)
{
	struct timeval tv;

	tv.tv_sec = 0;  /* 30 Secs Timeout */
	tv.tv_usec = timeout_ms * 1000;  // Not init'ing this can cause strange errors

	setsockopt(n->my_socket, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
	int	rc = write(n->my_socket, buffer, len);
	return rc;
}


void LWIP_disconnect(Network* n)
{
	close(n->my_socket);
}


void NewNetwork(Network* n)
{
	n->my_socket = 0;
	n->mqttread = LWIP_read;
	n->mqttwrite = LWIP_write;
	n->disconnect = LWIP_disconnect;
}


int CreateTCPConnect(char *srv, int port)
{
	int sockfd, error;
	struct sockaddr_in servaddr;
	socklen_t len;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) return sockfd;
	
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port);
	inet_aton(srv, &(servaddr.sin_addr));
	// TODO: Use gethostbyname to resolute the address of the host
	// iwip_gethostbyname_r
	// inet_pton(AF_INET, srv, &servaddr.sin_addr);
	
	error = connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	if (error == 0)
	{
		error = getsockname(sockfd, (struct sockaddr *)&servaddr, &len);
		if (error  >= 0)		
			return sockfd;
	}
	else return error;
}

int ConnectNetwork(Network* n, char* addr, int port)
{
	
	return n->my_socket = CreateTCPConnect(addr, port);
}	

#if (0)
	int type = SOCK_STREAM;
	struct sockaddr_in address;
	int rc = -1;
	
	
	sa_family_t family = AF_INET;
	struct addrinfo *result = NULL;
	struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};

	if ((rc = getaddrinfo(addr, NULL, &hints, &result)) == 0)
	{
		struct addrinfo* res = result;

		/* prefer ip4 addresses */
		while (res)
		{
			if (res->ai_family == AF_INET)
			{
				result = res;
				break;
			}
			res = res->ai_next;
		}

		if (result->ai_family == AF_INET)
		{
			address.sin_port = htons(port);
			address.sin_family = family = AF_INET;
			address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;
		}
		else
			rc = -1;

		freeaddrinfo(result);
	}

	if (rc == 0)
	{
		n->my_socket = socket(family, type, 0);
		if (n->my_socket != -1)
		{
			int opt = 1;			
			rc = connect(n->my_socket, (struct sockaddr*)&address, sizeof(address));
		}
	}

	return rc;
}
#endif
