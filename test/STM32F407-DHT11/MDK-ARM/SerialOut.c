#include <stdio.h>
#include <stdarg.h>

#include "utask.h"

#define CON_RCV_SIZE	60
#define PRN_BUF_SIZE	200

char gTmp[PRN_ONE_SIZE];

// U3 Send
static char sPrnBuffer[PRN_BUF_SIZE];
static int spTail = 0;
volatile static int spFull = 0, spHead = 0; 

// Semaphores
xSemaphoreHandle xPrnDMAMutex;

// UART1: Modbus
// UART3: Text console: Giveup control as soon as possible
extern UART_HandleTypeDef huart1, huart3;

extern void ModSendComplete(portBASE_TYPE *h);

extern volatile int gGPRSSending;

//=======================================================
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *UartHandle)
{
	static portBASE_TYPE	xHigher;

	xHigher = 0;
	
	if (UartHandle == &huart3) {
		xSemaphoreGiveFromISR(xPrnDMAMutex, &xHigher);	// MUTEX should NOT be used in ISR! use 0/1 counting instead!
	}
//	else if (UartHandle == &huart1)	{	// Modbus
//		// TODO
//		return;
//		//ModSendComplete(&xHigher);
//	}

	portYIELD_FROM_ISR(xHigher);  
}

int ReceiveGPRS(char *buf, int *pLen)
{
	return 0;
}

//=================================================
//
// UART3 Receive (CR+LF)
//
/*
int ConsoleRxISR(void)
{
	unsigned char ucByte;
	static portBASE_TYPE	xHigher;
	
	HAL_UART_Receive(&huart3, &ucByte, 1, 1);
//	HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
	
	if (u3rState == 0) {
		if (u3RcvPos < CON_RCV_SIZE) u3RcvBuf[u3RcvPos++] = ucByte;
		else u3rState = 2;
		if (ucByte == 10) 
		{ 
			u3rState = 1;	// LF received
			xSemaphoreGiveFromISR(xConSema, &xHigher);
			portYIELD_FROM_ISR(xHigher); 
		}
	}
	else {
	}
	return 0;
}
*/ 
//
// Get input from UART console buffer
// Paras: 
//	s,    pointer to the buffer
// pLen,  pointer to the count of content length
// Returns:
//   1:   frame got, the length of the frame (with CR/LF, not NULL terminated)
//	 0:	  no frame waiting
//  -1:   overruns (the pLen also available)
//  -2:   Parameter error
//  -3:   In processing
// neg:	  other error
/*
int GetConsoleInput(char **s, int *pLen)
{
	unsigned short temp;
	if (s == 0 || pLen == 0) return -2;

	if (u3rState == 0) return 0;
	if (u3rState == 3) return -3;
	temp = u3rState;
	u3rState = 3;	// Also reset the state even if it is in error
	
	*s = (char *)u3RcvBuf;
	*pLen = u3RcvPos;
	if (temp == 1) return 1;
	if (temp == 2) return -1;
	return -4;
}

//
int EnableConsole(void)
{
	if (u3rState == 3)
	{
		u3RcvPos = 0;
		u3rState = 0;
		return 1;
	}
	else return -1;
}
*/

//=================================================
//
// Console Send (UART3)
//
int Print(const char *s)
{
	int size = 0, head = spHead;
	if (spFull != 0) return 0;

	ENTER_CRITICAL_SECTION();
	
	while (size < PRN_BUF_SIZE-1)
	{
		if (*s == 0) break;
		sPrnBuffer[head++] = *s++;
		size++;
		if (head == PRN_BUF_SIZE) head = 0;
		if (head == spTail) { spFull = 1; break; }	// spTail is not necessary to be volatile
	}
	spHead = head;

	EXIT_CRITICAL_SECTION();
	
	xTaskNotifyGive(xPrnHandle);
	return size;
}

static void SendBufferBlockedDMA3(const char *s, int len)
{
	HAL_UART_Transmit_DMA(&huart3, (uint8_t *)s, len);
	xSemaphoreTake(xPrnDMAMutex, 5000);		// wait till finished
}

void vDaemonPrint(void *pvPara)
{
	int size;

	while(1)
	{
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
		//xSemaphoreTake(xPrnSema, 5000);
		while (spHead != spTail || spFull != 0)
		{
			size = spHead - spTail;
			if (size < 0) size += PRN_BUF_SIZE;
			if (size > PRN_ONE_SIZE || spFull == 1) size = PRN_ONE_SIZE;
			if (size + spTail >= PRN_BUF_SIZE) size = PRN_BUF_SIZE - spTail;
			SendBufferBlockedDMA3(sPrnBuffer+spTail, size);			// There will be 0.511ms between two calls
																	// Why? context switch? 
			spTail += size;
			if (spTail >= PRN_BUF_SIZE) spTail = 0;		
			spFull = 0;
		}
	}
}

int dbgWrite(const char *s, uint32_t ulIdx)
{
	int size = 0, head = spHead;
	if (spFull != 0) return 0;
	if (ulIdx >= PRN_BUF_SIZE-1) ulIdx = PRN_BUF_SIZE-1;
	
//	xSemaphoreTake(xPrnMutex, 1000);
	ENTER_CRITICAL_SECTION();
	
	while (size < ulIdx)
	{
		size++;
		sPrnBuffer[head] = *s;
		head++; s++;
		if (head >= PRN_BUF_SIZE) head = 0;
		if (head == spTail) { spFull = 1; break; }	// spTail is not necessary to be volatile
	}
	spHead = head;
	
	EXIT_CRITICAL_SECTION();

	xTaskNotifyGive(xPrnHandle);	
//	xSemaphoreGive(xPrnSema);	
//	xSemaphoreGive(xPrnMutex);
	return size;	
}

//============================
const char * const g_pcHex = "0123456789abcdef";   

// TODO: LIMITATION 32 chars!!!
void dbgPrintf(const char *pcString, ...)
{
	uint32_t ulIdx;
	uint32_t ulValue;
	uint32_t ulPos, ulCount;
	uint32_t ulBase;
	uint32_t ulNeg;
	char *pcStr;
	char pcBuf[32];
	uint8_t cFill;
	
	va_list vaArgP;
	va_start(vaArgP, pcString);
	while (*pcString)
	{
		for (ulIdx = 0; (pcString[ulIdx] != '%') && (pcString[ulIdx] != '\0'); ulIdx++) { } 
		dbgWrite(pcString, ulIdx); 
		pcString += ulIdx;
		if (*pcString == '%') 
		{
			pcString++;
			ulCount = 0;
			cFill = ' ';    
		again:    
			switch (*pcString++)
			{
			case '0': case '1': case '2': case '3': case '4': 
			case '5': case '6': case '7': case '8': case '9': 
				if ((pcString[-1] == '0') && (ulCount == 0)) cFill = '0';    
				ulCount *= 10; 
				ulCount += pcString[-1] - '0'; 
				goto again; 
			case 'c':
				ulValue = va_arg(vaArgP, unsigned long); 
				dbgWrite((char *)&ulValue, 1); 
				break;
			case 'd':
				ulValue = va_arg(vaArgP, unsigned long); 
				ulPos = 0; 	ulBase = 10; 
				if ((long)ulValue < 0) {ulValue = -(long)ulValue; ulNeg = 1; }  
				else ulNeg = 0;    
				goto convert; 
			case 's': 
				pcStr = va_arg(vaArgP, char *); 
				for (ulIdx = 0; pcStr[ulIdx] != 0; ulIdx++) {} 
				dbgWrite(pcStr, ulIdx); 
				if(ulCount > ulIdx) 
				{
					ulCount -= ulIdx; 
					while (ulCount--) dbgWrite(" ", 1);   
				} 
				break; 
			case 'u': 
				ulValue = va_arg(vaArgP, unsigned long); 
				ulPos = 0; 
				ulBase = 10; 
				ulNeg = 0;  
				goto convert; 
			case 'x': case 'X': case 'p': 
				ulValue = va_arg(vaArgP, unsigned long); 
				ulPos = 0; ulBase = 16; ulNeg = 0;    
		convert: 
				for (ulIdx = 1;  (((ulIdx * ulBase) <= ulValue) &&(((ulIdx * ulBase) / ulBase) == ulIdx)); ulIdx *= ulBase, ulCount--)  {}       
				if (ulNeg) ulCount--;                          
				if (ulNeg && (cFill == '0')) { pcBuf[ulPos++] = '-';  ulNeg = 0;  }
				if ((ulCount > 1) && (ulCount < 16)) 
					for (ulCount--; ulCount; ulCount--)  
						pcBuf[ulPos++] = cFill;    
				if (ulNeg) pcBuf[ulPos++] = '-'; 
				for (; ulIdx; ulIdx /= ulBase)    
					pcBuf[ulPos++] = g_pcHex[(ulValue / ulIdx) % ulBase];   
				dbgWrite(pcBuf, ulPos); 
				break; 
			case '%':    
				dbgWrite(pcString - 1, 1);                      
				break;    
			default: 
				dbgWrite(pcString - 1, 1);
				break;    
			} // switch    
		}    // if
	} // while    
	va_end(vaArgP);    
}
