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
  * @brief  ��ʼ��IO�ںͲ���
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
  * @brief  ��ȡ8bit ����
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
		//��ȡ50us�ĵ͵�ƽ
		cout=1;
		while(!DHT_ReadPin() && cout++);
		
		//��ʱ30us֮���ȡIO�ڵ�״̬
		HAL_DelayUs(30);
		
		//�Ȱ��ϴε�������λ���ٱ��汾�ε�����λ
		data = data << 1;
		
		if(DHT_ReadPin() == GPIO_PIN_SET)
		{	
			data |= 1;
		}		
		
		//�ȴ�������ǵ͵�ƽ��������һλ���ݽ���
		cout=1;
		while(DHT_ReadPin() && cout++);
	}

	return data;
}

/**
  * @brief  ��ȡ40bit����
  * @param  none.
  * @retval 1 ��ȡ�ɹ���0��ȡʧ��.
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
  * @brief  ��ȡ�¶�
  * @param  none.
  * @retval Temp, �¶�ֵ
  */
int DHT11_GetTem(void)
{
	return (DHT11.Tem_H << 8 | DHT11.Tem_L);
}

/**
  * @brief  ��ȡʪ��
  * @param  none.
  * @retval Hum,ʪ��ֵ
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

