#include "stFlash.h" 
#include "utask.h"

#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256
#define W25X_WriteEnable		            0x06 
#define W25X_WriteDisable		            0x04 
#define W25X_ReadStatusReg		          0x05 
#define W25X_WriteStatusReg		          0x01 
#define W25X_ReadData			              0x03 
#define W25X_FastReadData		            0x0B 
#define W25X_FastReadDual		            0x3B 
#define W25X_PageProgram		            0x02 
#define W25X_BlockErase			            0xD8 
#define W25X_SectorErase		            0x20 
#define W25X_ChipErase			            0xC7 
#define W25X_PowerDown			            0xB9 
#define W25X_ReleasePowerDown	          0xAB 
#define W25X_DeviceID			              0xAB 
#define W25X_ManufactDeviceID   	      0x90 
#define W25X_JedecDeviceID		          0x9F 

#define WIP_Flag                        0x01  /* Write In Progress (WIP) flag */

#define Dummy_Byte                      0xFF

extern SPI_HandleTypeDef hspi2;

/**send 1byte, read 1byte(always 0xFF)**/
uint8_t SPI_FLASH_SendByte(uint8_t byte)
{
  uint8_t d_read, d_send=byte;
  if(HAL_SPI_TransmitReceive(&hspi2, &d_send, &d_read, 1, 0xFFFFFF)!=HAL_OK)
    d_read=Dummy_Byte;
  
  return d_read; 
}
/**send 1byte(always 0xFF), read 1byte**/
uint8_t SPI_FLASH_ReadByte(void)
{
  uint8_t d_read, d_send=Dummy_Byte;
  if(HAL_SPI_TransmitReceive(&hspi2, &d_send, &d_read, 1, 0xFFFFFF)!=HAL_OK)
    d_read=Dummy_Byte;
  
  return d_read;    
}

uint32_t SPI_FLASH_ReadID(void)
{
  uint32_t Temp = 0, Temp0 = 0, Temp1 = 0, Temp2 = 0;

  FLASH_SPI_CS_ENABLE();
  SPI_FLASH_SendByte(W25X_JedecDeviceID);
  Temp0 = SPI_FLASH_SendByte(Dummy_Byte);
  Temp1 = SPI_FLASH_SendByte(Dummy_Byte);
  Temp2 = SPI_FLASH_SendByte(Dummy_Byte);
  FLASH_SPI_CS_DISABLE();
  
	Temp = (Temp0 << 16) | (Temp1 << 8) | Temp2;
  return Temp;
}

uint32_t SPI_FLASH_ReadDeviceID(void)
{
  uint32_t Temp = 0;

  FLASH_SPI_CS_ENABLE();

  SPI_FLASH_SendByte(W25X_DeviceID);
  SPI_FLASH_SendByte(Dummy_Byte);
  SPI_FLASH_SendByte(Dummy_Byte);
  SPI_FLASH_SendByte(Dummy_Byte);
  
  Temp = SPI_FLASH_SendByte(Dummy_Byte);

  FLASH_SPI_CS_DISABLE();

  return Temp;
}

void SPI_FLASH_WriteEnable(void)
{
  FLASH_SPI_CS_ENABLE();
  SPI_FLASH_SendByte(W25X_WriteEnable);
  FLASH_SPI_CS_DISABLE();
}


