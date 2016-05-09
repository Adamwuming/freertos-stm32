#ifndef _SBY_TASKS_
#define _SBY_TASKS_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"

#define UART3_RX_BUF_SIZE                     	256
//#define SPI1_READY_BUF_SIZE											128

#define BUFSIZE		800
#define RBUFSIZE	400
#define kPort		1883
#define MAXTOPIC	125
#define kQOS		QOS1

extern UART_HandleTypeDef huart3;
extern TIM_HandleTypeDef htim6, htim7;

extern char UART3_RxBuf[];
extern int UART3_RxBuf_Index;
extern osSemaphoreId recFlagHandle;

///////////////////////////////////////////////////

static const char *kUserid = "570da6256097e943626e1245";
static const char *kToken  = "UDlvRhCaUrIQEQcCwAvKMuYOsLHMNICO";
static const char *kServer = "139.198.0.174";

extern 	int tem, hum;

extern xQueueHandle xPubQueue;
extern void TCPClt(void *argu);
extern void Pub_SENSOR(void *argu);
extern void StartPacketsProcess(void * argument);
////////////////////////////////////////////////////////
extern void HAL_CalTick(void);
extern void HAL_DelayUs(int nDelay);
extern void PrintHex(char *s, unsigned char *buffer, int num);

extern void LED_On(int Led);
extern void LED_Off(int Led);
extern void LED_Toggle(int Led);

extern void MyTimerEnable(TIM_HandleTypeDef *htim);
extern void MyTimerDisable(TIM_HandleTypeDef *htim);

#endif
