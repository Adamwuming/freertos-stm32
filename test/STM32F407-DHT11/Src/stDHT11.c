/* Includes ------------------------------------------------------------------*/
#include <string.h>
#include <stdlib.h>

#include "utask.h" // NOTE:
#include "stFlash.h"

#define DHT11_GPIO_Port    DH_DATA_GPIO_Port
#define DHT11_Pin          DH_DATA_Pin

#define DHT_HignPin()	HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_SET)
#define DHT_LowPin()	HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, GPIO_PIN_RESET)
#define DHT_ReadPin()	HAL_GPIO_ReadPin(DHT11_GPIO_Port, DHT11_Pin)

struct DHT11 gDHT[1];

extern int gEInterval;

/* one of "0" -> write 1 struct DHT data to flash*/
unsigned int getAddrOffset(unsigned int addr)
{
	unsigned char Rx_Buffer[SPI_FLASH_PageSize];
	
	SPI_FLASH_BufferRead(Rx_Buffer, addr, SPI_FLASH_PageSize);
	return BitLow_Count(Rx_Buffer, SPI_FLASH_PageSize);
}

int modifyAddrOffset(unsigned int addr)
{
	unsigned int offset, i;
	uint8_t Tx_Buffer[8] = {0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, 0x00};
	
	offset=getAddrOffset(addr);
	if(offset<SPI_FLASH_PageSize*8)
	{
		switch(offset%8)
		{
		case	0: i=0; break ;
		case	1: i=1; break ;
		case	2: i=2; break ;
		case	3: i=3; break ;
		case	4: i=4; break ;
		case	5: i=5; break ;
		case	6: i=6; break ;
		case	7: i=7; break ;
		}
		SPI_FLASH_BufferWrite(Tx_Buffer+i, addr + offset/8, 1);
		printf("Offset has changed on addr->0x%x. New Offset->%d\n", addr, getAddrOffset(addr));
		return 0;
	}
	else 
	{
		printf("Offset has not changed on addr->0x%x.", addr);
		return -1;
	}

}

int WriteDHTFlash(unsigned char *pTxData)
{
	unsigned int offset_r, offset_w;
	
	WriteStart:
	offset_r=getAddrOffset(DHT_Flash_Read_Offset_Addr);
	offset_w=getAddrOffset(DHT_Flash_Write_Offset_Addr);

	if(offset_w<(SPI_FLASH_PerBlockSize/DHT_DATA_BYTE_SIZE)*2)
	{
		SPI_FLASH_BufferWrite(pTxData, DHT_Flash_Base_Addr + offset_w*DHT_DATA_BYTE_SIZE, DHT_DATA_BYTE_SIZE);
		printf("Write DHT data to flash addr->0x%x.\n", DHT_Flash_Base_Addr + offset_w*DHT_DATA_BYTE_SIZE);
		modifyAddrOffset(DHT_Flash_Write_Offset_Addr);
		return 0;
	}		
	else 
	{
		if(offset_r==offset_w)
		{
			printf("Sector Erase: addr->0x%x\n", DHT_Flash_Base_Addr);
			SPI_FLASH_SectorErase(DHT_Flash_Base_Addr);
			printf("Sector Erase: addr->0x%x\n", DHT_Flash_Base_Addr+0x001000);
			SPI_FLASH_SectorErase(DHT_Flash_Base_Addr+0x001000);
			printf("Sector Erase: addr->0x%x\n", DHT_Flash_Base_Addr+0x002000);
			SPI_FLASH_SectorErase(DHT_Flash_Base_Addr+0x002000);
			printf("Reset pointer offset value, offset_r %d->%d, offset_w %d->%d\n", offset_r, getAddrOffset(DHT_Flash_Read_Offset_Addr), offset_w, getAddrOffset(DHT_Flash_Write_Offset_Addr));
			goto WriteStart;
		}
		else
			printf("Can't write DHT data to flash addr->0x%x. It's the most offset_r->%d offset_w->%d\n", DHT_Flash_Base_Addr + offset_w*DHT_DATA_BYTE_SIZE, offset_r, offset_w);
			return -1;
	}
}

int ReadDHTFlash(unsigned char *pRxData)
{
	unsigned int offset_r, offset_w;
	
	offset_r=getAddrOffset(DHT_Flash_Read_Offset_Addr);
	offset_w=getAddrOffset(DHT_Flash_Write_Offset_Addr);
	
	if(offset_r<offset_w)
	{
		SPI_FLASH_BufferRead(pRxData, DHT_Flash_Base_Addr + offset_r*DHT_DATA_BYTE_SIZE, DHT_DATA_BYTE_SIZE);
		printf("Read DHT data from flash addr->0x%x. New offset_r->%d\n", DHT_Flash_Base_Addr + offset_r*DHT_DATA_BYTE_SIZE, getAddrOffset(DHT_Flash_Read_Offset_Addr));
		return 0;
	}
	else 
	{
		printf("Can't read DHT data from flash addr->0x%x. It's the most offset_r->%d\n", DHT_Flash_Base_Addr + offset_r*DHT_DATA_BYTE_SIZE, getAddrOffset(DHT_Flash_Read_Offset_Addr));
		return -1;
	}	
}


void initDHT(void)
{
	gDHT->pickTime=0;
	gDHT->pickTem=0;
	gDHT->pickHum=0;
	printf("Init gDHT.\n");
}

void wDHT(int pickTime, int pickTem, int pickHum)
{
	int sock=1;
	
	gDHT->pickTime=pickTime;
	gDHT->pickTem=pickTem;
	gDHT->pickHum=pickHum;
	printf("gTem: %d %d %d\n", gDHT->pickTime, gDHT->pickTem, gDHT->pickHum);
	
	if(!gConnect)
	{
		WriteDHTFlash((uint8_t *)gDHT);
		initDHT();
	}
	else	
		xQueueSendToBack(xPubQueue, &sock, 0);	//	json type, sock=1 
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
		while(!DHT_ReadPin() && cout++)		
		{
			HAL_DelayUs(1);
			if(cout>100) break;
		}

		HAL_DelayUs(30);
		data = data << 1;
		if(DHT_ReadPin() == GPIO_PIN_SET)
		{	
			data |= 1;
		}
		
		cout=1;
		while(DHT_ReadPin() && cout++)
		{
			HAL_DelayUs(1);
			if(cout>100) break;
		}
	}
	return data;
}


void DHT11_ReadData(void)
{
	unsigned int cout = 1;
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
		while(!DHT_ReadPin() && cout++)
		{
			HAL_DelayUs(1);
			if(cout>100) break;
		}

		cout = 1;
		while(DHT_ReadPin() && cout++)		
		{
			HAL_DelayUs(1);
			if(cout>100) break;
		}

		H_H = DH11_ReadByte();
		H_L = DH11_ReadByte();
		T_H = DH11_ReadByte();
		T_L = DH11_ReadByte();
		Check = DH11_ReadByte();
		if(Check == (H_H + H_L + T_H + T_L))
		{
			wDHT(HAL_GetTick(), T_H, H_H);
		}
	}
}

void DHT11_Task(void *argu)
{
	int tickstart = 0;
	int sock = 3;
	tickstart = HAL_GetTick();
	UNUSED(argu);

  /* Infinite loop */
  for(;;)
  {
		osDelay(1);
		if ((HAL_GetTick() - tickstart) >= 1000 * gEInterval){
			LED_Toggle(2);
			DHT11_ReadData();
			tickstart = HAL_GetTick();
			xQueueSendToBack(xPubQueue, &sock, 0);	//	json type, sock=3
		}
  }
}

