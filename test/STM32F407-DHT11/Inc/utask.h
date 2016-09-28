#ifndef _YMT_TASKS_
#define _YMT_TASKS_

#include "stm32f4xx_hal.h"
#include "cmsis_os.h"
#define GPIO_MFRC 1
//#define SPI_MFRC 1

/* ----------------------- Defines ------------------------------------------*/
#define __BUILD_	0x160920
#define PROMPT		"\n> "

#define GPIO_SET(port,pin) 		(port->BSRR = pin)
#define GPIO_CLR(port,pin) 		(port->BSRR = pin<< 16U)
#define GPIO_STAT(port,pin)   (!!(port->IDR & pin))

typedef enum {
	BAUD9600 = 0,
	NAUD14400,
	BAUD19200,
	BAUD38400,
	BAUD57600,
	BAUD115200,
} BaudEnum;

/*Json payload type*/
#define PUB_TYPE_AGENT                      0
#define PUB_TYPE_DHT                        1
#define PUB_TYPE_HISTORY_DHT                2
#define PUB_TYPE_INV                        3

/*FreeRTOS*/
extern xTaskHandle xPrnHandle, xMQTTHandle, xDHTHandle, xMBPHandle, \
	xCmdAnalyzeHandle, xRFIDPollHandle;
extern xQueueHandle xPubQueue;
extern xSemaphoreHandle xPrnDMAMutex;

/*ST Peripheral*/
#define ENTER_CRITICAL_SECTION( )   __disable_irq()
#define EXIT_CRITICAL_SECTION( )    __enable_irq()
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart3;

/*Print*/
#define PRN_ONE_SIZE	128
#define CON_RCV_SIZE	60
#define PRN_BUF_SIZE	500

extern char gTmp[PRN_ONE_SIZE];
extern int Print(const char *s);

/*Misc Func*/
extern void HAL_CalTick(void);
extern void HAL_DelayUs(int nDelay);

extern unsigned int BitHign_Count(unsigned char *v, int size);
extern unsigned int BitLow_Count(unsigned char *v, int size);
extern void LED_On(int Led);
extern void LED_Off(int Led);
extern void LED_Toggle(int Led);

/*DHT11 + Flash*/
struct DHT11
{
  /*STRUCT 12byte*/
	int pickTem;		//Temperature integer part
	int pickHum;		//Humidity integer part
	int pickTime;
};

extern int gConnect;

extern struct DHT11 gDHT[1];
extern void initDHT(void);

/*42byte offset = 341 * DHT_DATA_BYTE_SIZE = 4KB data */
#define DHT_Flash_Base_Addr						0
#define DHT_Flash_Write_Offset_Addr		0x002A00
#define DHT_Flash_Read_Offset_Addr		0x002B00
#define DHT_DATA_BYTE_SIZE 						(3*4)

extern void wDHT(int pickTime, int pickTem, int pickHum);
extern unsigned int getAddrOffset(unsigned int addr);
extern int modifyAddrOffset(unsigned int addr);
extern int WriteDHTFlash(unsigned char *pTxData);
extern int ReadDHTFlash(unsigned char *pRxData);

/*INV Flash*/
#endif
