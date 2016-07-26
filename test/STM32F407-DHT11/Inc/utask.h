#ifndef _SBY_TASKS_
#define _SBY_TASKS_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#include <stdio.h>

/*task*/
extern void MQTTWork(void *argu);
extern void DHT11_Task(void *argu);

extern xQueueHandle xPubQueue;

/*Private variables*/
extern UART_HandleTypeDef huart3;

/*func*/
extern void HAL_CalTick(void);
extern void HAL_DelayUs(int nDelay);

extern void LED_On(int Led);
extern void LED_Off(int Led);
extern void LED_Toggle(int Led);

/*DHT11*/
struct DHT11
{
	int pickTem;		//Temperature integer part
	int pickHum;		//Humidity integer part
	int pickTime;
	
};
extern struct DHT11 gDHT[100];
extern void wDHT(int pickTime, int pickTem, int pickHum);
extern void initDHT(int i);

#endif
