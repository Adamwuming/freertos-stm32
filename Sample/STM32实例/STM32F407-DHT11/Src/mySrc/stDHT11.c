/* Includes ------------------------------------------------------------------*/
#include "stDHT11.h"
#include "utask.h"
#include <string.h>
#include <stdlib.h>
#define DHT_HignPin()	HAL_GPIO_WritePin(DHT11_DATA_PIN_GPIO_Port, DHT11_DATA_PIN_Pin, GPIO_PIN_SET)
#define DHT_LowPin()	HAL_GPIO_WritePin(DHT11_DATA_PIN_GPIO_Port, DHT11_DATA_PIN_Pin, GPIO_PIN_RESET)
#define DHT_ReadPin()	HAL_GPIO_ReadPin(DHT11_DATA_PIN_GPIO_Port, DHT11_DATA_PIN_Pin)

int tem, hum;

DHT11_TypeDef DHT11;

xQueueHandle xPubQueue;

static void DHT_Set_Output(void);
static void DHT_Set_Input(void);	


void DHT_Set_Output(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin = DHT11_DATA_PIN_Pin;				 
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
  GPIO_InitStruct.Pull = GPIO_PULLUP;	
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(DHT11_DATA_PIN_GPIO_Port, &GPIO_InitStruct);			 					
}	

void DHT_Set_Input(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;
	GPIO_InitStruct.Pin = DHT11_DATA_PIN_Pin;				 
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(DHT11_DATA_PIN_GPIO_Port, &GPIO_InitStruct);			 
}

/**
  * @brief  初始化IO口和参数
  * @param  none.
  * @retval none.
  */
void DHT11_Init(void)
{	
	DHT11.Tem_H = 0;
	DHT11.Tem_L = 0;
	DHT11.Hum_H = 0;
	DHT11.Hum_L = 0;
}

/**
  * @brief  读取8bit 数据
  * @param  none.
  * @retval none.
  */
int DH21_ReadByte(void)
{
	int data=0;
	char i;
	char cout;
	
	for(i=0; i<8; i++)
	{
		//读取50us的低电平
		cout=1;
		while(!DHT_ReadPin() && cout++);
		
		//延时30us之后读取IO口的状态
		HAL_DelayUs(30);
		
		//先把上次的数据移位，再保存本次的数据位
		data = data << 1;
		
		if(DHT_ReadPin() == GPIO_PIN_SET)
		{	
			data |= 1;
		}		
		
		//等待输入的是低电平，进入下一位数据接收
		cout=1;
		while(DHT_ReadPin() && cout++);
	}

	return data;
}

/**
  * @brief  读取40bit数据
  * @param  none.
  * @retval 1 读取成功，0读取失败.
  */
int DHT11_ReadData(void)
{
	int i;
	unsigned int cout = 1;
	unsigned int T_H, T_L, H_H, H_L, Check;
	
	DHT_Set_Output();
	DHT_HignPin();
	osDelay(20);
	
	DHT_LowPin();
	osDelay(30);		
	
	DHT_HignPin();		
	HAL_DelayUs(2);	
	
	DHT_Set_Input();
	HAL_DelayUs(60);
	
	i = DHT_ReadPin();
	
	if (i == 0)
	{
		cout = 1;
		while(!DHT_ReadPin() && cout++);
				
		cout = 1;
		while(DHT_ReadPin() && cout++);
		
		H_H = DH21_ReadByte();
		H_L = DH21_ReadByte();	
		T_H = DH21_ReadByte();
		T_L = DH21_ReadByte();
		Check = DH21_ReadByte();
		if(Check == (H_H + H_L + T_H + T_L))
		{
			DHT11.Hum_H = H_H;
			DHT11.Hum_L = H_L;
			DHT11.Tem_H = T_H;
			DHT11.Tem_L = T_L;
			return 1;
		}	
				else
		{
			return 0;
		}
	}
		return 0;
	
//	__HAL_GPIO_EXTI_CLEAR_IT(DHT11_DATA_PIN_Pin);
//	HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);
}

/**
  * @brief  获取温度
  * @param  none.
  * @retval Temp, 温度值
  */
int DHT11_GetTem(void)
{
	return (DHT11.Tem_H << 8 | DHT11.Tem_L);
}

/**
  * @brief  获取湿度
  * @param  none.
  * @retval Hum,湿度值
  */
int DHT11_GetHum(void)
{
	return (DHT11.Hum_H << 8 | DHT11.Hum_L);
}

void __DHT11_IRQHandler(void)
{
	unsigned int T_H, T_L, H_H, H_L, Check;
		
	H_H = DH21_ReadByte();
	H_L = DH21_ReadByte();	
	T_H = DH21_ReadByte();
	T_L = DH21_ReadByte();
	Check = DH21_ReadByte();
	if(Check == (H_H + H_L + T_H + T_L))
	{
		DHT11.Hum_H = H_H;
		DHT11.Hum_L = H_L;
		DHT11.Tem_H = T_H;
		DHT11.Tem_L = T_L;	
	}
		NVIC_DisableIRQ(EXTI9_5_IRQn);
}

void Pub_SENSOR(void *argu)
{
	int i;
	UNUSED(argu);
	
  /* Infinite loop */
  for(;;)
  {
	osDelay(10000);
	i = DHT11_ReadData();
	if(i != 0){
		tem = DHT11_GetTem();
		hum = DHT11_GetHum();
		LED_Toggle(3);
		xQueueSendToBack(xPubQueue, &i, 0);
	}

  }
}

