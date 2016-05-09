#ifndef _DHT11_H
#define _DHT11_H

typedef struct _DHT11
{
	char Tem_H;		//�¶���������
	char Tem_L;		//�¶�С������
	char Hum_H;		//ʪ����������
	char Hum_L;		//ʪ��С������
	
}DHT11_TypeDef;

/* ���� DHT11 ���� */
#define DHT_RCC			RCC_APB2Periph_GPIOC
#define DHT_GPIO		GPIOC

/**
  * @brief  ��ʼ��IO�ںͲ���
  * @param  none.
  * @retval none.
  */
void DHT11_Init(void);


/**
  * @brief  ��ȡ40bit����
  * @param  none.
  * @retval 1 ��ȡ�ɹ���0��ȡʧ��.
  */
int DHT11_ReadData(void);

/**
  * @brief  ��ȡ�¶�
  * @param  none.
  * @retval Temp, �¶�ֵ���߰�λΪ�������֣��Ͱ�λΪС������
  */
int DHT11_GetTem(void);

/**
  * @brief  ��ȡʪ��
  * @param  none.
  * @retval Hum,ʪ��ֵ,�߰�λΪ�������֣��Ͱ�λΪС������
  */
int DHT11_GetHum(void);

#endif

