/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>

#include "utask.h" // NOTE:

#define DHT11_GPIO_Port    DH_DATA_GPIO_Port
#define DHT11_Pin          DH_DATA_Pin

#define DHT_HignPin()	HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET)
#define DHT_LowPin()	HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)
#define DHT_ReadPin()	HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)

struct DHT11 gDHT[100];

extern int gEInterval;

void wDHT(int pickTime, int pickTem, int pickHum)
{
	int i;
	struct DHT11 *p;
	
	p=gDHT;
	for(i=0;i<100;i++)
	{
		p=gDHT+i;
		if((*p).pickTem==0) break;
	}
	(*p).pickTime=pickTime;
	(*p).pickTem=pickTem;
	(*p).pickHum=pickHum;
	printf("i: %d gTem: %d %d %d\n", i, (*p).pickTime, (*p).pickTem, (*p).pickHum);
}

void initDHT(int i)
{
	struct DHT11 *p;
	p=gDHT+i;
	(*p).pickTime=0;
	(*p).pickTem=0;
	(*p).pickHum=0;
}

void DHT_Set_Output(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin = DHT11_Pin;				 
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;	
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);			 					
}	

void DHT_Set_Input(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin = DHT11_Pin;				 
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(DHT11_GPIO_Port, &GPIO_InitStruct);			 
}

int DH11_ReadByte(void)
{
	unsigned int data=0, i, cout;
	
	for(i=0; i<8; i++)
	{
		cout=1;
		while(!DHT_ReadPin() && cout++);

		HAL_DelayUs(30);
		data = data << 1;
		if(DHT_ReadPin() == GPIO_PIN_SET)
		{	
			data |= 1;
		}
		
		cout=1;
		while(DHT_ReadPin() && cout++);
	}
	return data;
}


void DHT11_ReadData(void)
{
	unsigned int cout = 1;
	short sock=1;
	unsigned int T_H, T_L, H_H, H_L, Check;
	
	DHT_Set_Output();
	
	DHT_LowPin();
	osDelay(20);		
	
	DHT_HignPin();		
	HAL_DelayUs(20);	//stMisc.c
	
	DHT_Set_Input();

	if (DHT_ReadPin() == 0)
	{
		cout = 1;
		while(!DHT_ReadPin() && cout++);

		cout = 1;
		while(DHT_ReadPin() && cout++);

		H_H = DH11_ReadByte();
		H_L = DH11_ReadByte();
		T_H = DH11_ReadByte();
		T_L = DH11_ReadByte();
		Check = DH11_ReadByte();
		if(Check == (H_H + H_L + T_H + T_L)){
			wDHT(HAL_GetTick(), T_H, H_H);
			xQueueSendToBack(xPubQueue, &sock, 0);
		}
	}
}

void DHT11_Task(void *argu)
{
	int tickstart = 0;
	tickstart = HAL_GetTick();
	UNUSED(argu);

  /* Infinite loop */
  for(;;)
  {
		if ((HAL_GetTick() - tickstart) >= 1000 * gEInterval){
			LED_Toggle(2);
			DHT11_ReadData();
			tickstart = HAL_GetTick();
		}
  }
}

