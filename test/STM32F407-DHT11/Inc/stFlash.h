#ifndef _ST_FLASH_
#define _ST_FLASH_

#include "stm32f4xx_hal.h"

#define SPI_FLASH_PageSize              256
#define SPI_FLASH_PerWritePageSize      256
#define SPI_FLASH_PerBlockSize					(4*1024)

#define W25X_WriteEnable		            0x06
#define W25X_WriteDisable		            0x04
#define W25X_ReadStatusReg1		          0x05
#define W25X_ReadStatusReg2		          0x05
#define W25X_WriteStatusReg		          0x01
#define W25X_PageProgram		            0x02
#define W25X_SectorErase		            0x20
#define W25X_BlockErase32			          0x52
#define W25X_BlockErase64			          0xD8
#define W25X_ChipErase			            0xC7
#define W25X_EPSuspend									0x75
#define W25X_EPResume										0x7A
#define W25X_PowerDown			            0xB9
#define W25X_ReadData			              0x03
#define W25X_FastReadData		            0x0B
#define W25X_ReleasePowerDown	          0xAB
#define W25X_FastReadDual		            0x3B
#define W25X_DeviceID			              0xAB
#define W25X_ManufactDeviceID   	      0x90
#define W25X_JedecDeviceID		          0x9F

#define WIP_Flag                        0x01  /* Write In Progress (WIP) flag */
#define Dummy_Byte                      0xFF
#define SPI_FLASH_ID 		0xEF4014			//W25Q80

#define	FLASH_SPI_CS_ENABLE()	HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_RESET)
#define	FLASH_SPI_CS_DISABLE()	HAL_GPIO_WritePin(Flash_CS_GPIO_Port, Flash_CS_Pin, GPIO_PIN_SET)


void MX_SPIFlash_Init(void);
void SPI_FLASH_SectorErase(uint32_t SectorAddr);
void SPI_FLASH_BulkErase(void);
void SPI_FLASH_PageWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferWrite(uint8_t* pBuffer, uint32_t WriteAddr, uint16_t NumByteToWrite);
void SPI_FLASH_BufferRead(uint8_t* pBuffer, uint32_t ReadAddr, uint16_t NumByteToRead);
uint32_t SPI_FLASH_ReadID(void);
uint32_t SPI_FLASH_ReadManufactDeviceID(void);
void SPI_FLASH_StartReadSequence(uint32_t ReadAddr);
void SPI_Flash_PowerDown(void);
void SPI_Flash_WAKEUP(void);

uint8_t SPI_FLASH_ReadByte(void);
uint8_t SPI_FLASH_SendByte(uint8_t byte);
void SPI_FLASH_WriteEnable(void);
void SPI_FLASH_WaitForWriteEnd(void);

#endif
