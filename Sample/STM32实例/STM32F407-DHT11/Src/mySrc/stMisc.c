#include <stdio.h>
#include "utask.h"

int UART3_RxBuf_Index = 0;
char UART3_RxBuf[UART3_RX_BUF_SIZE];

static int usTick = 90;

void HAL_CalTick(void)
{
	usTick = HAL_RCC_GetSysClockFreq() / 1000000;
}

void HAL_DelayUs(int nDelay)
{
	int curCounter = 0, curTickVal;
	int preTickVal;
	
	nDelay = nDelay * usTick - usTick/2;

	preTickVal	= SysTick->VAL;
	for (;;)
	{
		curTickVal = SysTick->VAL;
		curCounter = curCounter + preTickVal - curTickVal;
		if (curTickVal >= preTickVal) curCounter += SysTick->LOAD;
		if (curCounter > nDelay) return;
		preTickVal = curTickVal;
	}
}


void PrintHex(char *s, unsigned char *buffer, int num)
{
	for (int i=0; i<num; i++)
		sprintf(s + 3*i, "%02x ", *(buffer+i));
}

void LED_On(int Led)
{
	switch(Led)
	{
		case 1:HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_SET); break;
		case 2:HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_SET); break;
		case 3:HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_SET); break;
		default: return;
	}
}

void LED_Off(int Led)
{
  switch(Led)
	{
		case 1:HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, GPIO_PIN_RESET); break;
		case 2:HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, GPIO_PIN_RESET); break;
		case 3:HAL_GPIO_WritePin(GPIOE, GPIO_PIN_9, GPIO_PIN_RESET); break;
		default: return;
	}
}

void LED_Toggle(int Led)
{
  switch(Led)
	{
		case 1:HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_7); break;
		case 2:HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_8); break;
		case 3:HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_9); break;
		default: return;
	}
}

void MyTimerEnable(TIM_HandleTypeDef *htim)
{
	__HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
	__HAL_TIM_ENABLE_IT(htim, TIM_IT_UPDATE);
	__HAL_TIM_SetCounter(htim, 0);
	__HAL_TIM_ENABLE(htim);
}

void MyTimerDisable(TIM_HandleTypeDef *htim)
{
	__HAL_TIM_DISABLE(htim);
	__HAL_TIM_SetCounter(htim, 0);
	__HAL_TIM_DISABLE_IT(htim, TIM_IT_UPDATE);
	__HAL_TIM_CLEAR_IT(htim, TIM_IT_UPDATE);
}

void HAL_TIM_UART_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
	static portBASE_TYPE xHigherPriorityTaskWoken; 
	MyTimerDisable(htim);
	xHigherPriorityTaskWoken = pdFALSE; 
	xSemaphoreGiveFromISR(recFlagHandle, &xHigherPriorityTaskWoken);	
}

void UART3RxIRQ(void)
{
	uint8_t	ch;
	HAL_UART_Receive(&huart3, &ch, 1, 1);
	UART3_RxBuf[UART3_RxBuf_Index] = ch;
	UART3_RxBuf_Index++;
	if(UART3_RxBuf_Index>(UART3_RX_BUF_SIZE-1))	
		UART3_RxBuf_Index=(UART3_RX_BUF_SIZE-1);
	MyTimerEnable(&htim7);
}

