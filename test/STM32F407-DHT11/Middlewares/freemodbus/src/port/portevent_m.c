/*
 * FreeModbus Libary: RT-Thread Port
 * Copyright (C) 2013 Armink <armink.ztl@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * File: $Id: portevent_m.c v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */
/* ----------------------- System includes ----------------------------------*/
#include <stdlib.h>

/* ----------------------- Platform includes --------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"
#include "port.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Defines ------------------------------------------*/
/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Variables ----------------------------------------*/
xQueueHandle xMBEventQueue;
xQueueHandle xMBReqQueue;
xQueueHandle xMasterRunResQueue;

static int mb_event;
static portBASE_TYPE xHigherPriorityTaskWoken;
/* ----------------------- Start implementation -----------------------------*/
BOOL
xMBMasterPortEventInit( void )
{
	  EnterCriticalSection(  );
		xMBEventQueue = xQueueCreate(1, sizeof(int));
		xMBReqQueue = xQueueCreate(1, sizeof(int));
		ExitCriticalSection(  );
	
    return TRUE;
}

BOOL
xMBMasterPortEventPost( eMBMasterEventType eEvent )
{
		EnterCriticalSection(  );
		mb_event = eEvent;

		xHigherPriorityTaskWoken = pdFALSE;
		if(mb_event <= EV_MASTER_ERROR_PROCESS)
		{
				//printf("Send xMBEventQueue: %d\n", mb_event);
			xQueueSendToBackFromISR(xMBEventQueue, &mb_event, &xHigherPriorityTaskWoken);
		}
		else
		{
				//printf("Send xMBReqQueue: %d\n", mb_event);
			xQueueSendToBackFromISR(xMBReqQueue, &mb_event, &xHigherPriorityTaskWoken);
		}
		ExitCriticalSection(  );
		
		return TRUE;
}

BOOL
xMBMasterPortEventGet( int *eEvent )
{
		int eQueue;
		xQueueReceive(xMBEventQueue, &eQueue, portMAX_DELAY);
	  //printf("Rec xMBEventQueue: %d\n", eQueue);
		switch (eQueue)
    {
    case EV_MASTER_READY:
        *eEvent = EV_MASTER_READY;
        break;
    case EV_MASTER_FRAME_RECEIVED:
        *eEvent = EV_MASTER_FRAME_RECEIVED;
        break;
    case EV_MASTER_EXECUTE:
        *eEvent = EV_MASTER_EXECUTE;
        break;
    case EV_MASTER_FRAME_SENT:
        *eEvent = EV_MASTER_FRAME_SENT;
        break;
    case EV_MASTER_ERROR_PROCESS:
        *eEvent = EV_MASTER_ERROR_PROCESS;
        break;
    }
		return TRUE;
}
/**
 * This function is initialize the OS resource for modbus master.
 * Note:The resource is define by OS.If you not use OS this function can be empty.
 *
 */
void vMBMasterOsResInit( void )
{
	xMasterRunResQueue = xQueueCreate(1, sizeof(int));
}

/**
 * This function is take Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be just return TRUE.
 *
 * @param lTimeOut the waiting time.
 *
 * @return resource taked result
 */
BOOL xMBMasterRunResTake( LONG lTimeOut )
{
    /*If waiting time is -1 .It will wait forever */
    //return rt_sem_take(&xMasterRunRes, lTimeOut) ? FALSE : TRUE ;
		//return xQueueReceive(xMBEventQueue, &eQueue, lTimeOut)?FALSE : TRUE;
	return TRUE;
}

/**
 * This function is release Mobus Master running resource.
 * Note:The resource is define by Operating System.If you not use OS this function can be empty.
 *
 */
void vMBMasterRunResRelease( void )
{
    /* release resource */
    //rt_sem_release(&xMasterRunRes);
}

/**
 * This is modbus master respond timeout error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBRespondTimeout(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) 
{
		mb_event = EV_MASTER_ERROR_RESPOND_TIMEOUT;

		xHigherPriorityTaskWoken = pdFALSE; 
		xQueueSendToBackFromISR(xMBReqQueue, &mb_event, &xHigherPriorityTaskWoken);

}

/**
 * This is modbus master receive data error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBReceiveData(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) 
{
		mb_event = EV_MASTER_ERROR_RECEIVE_DATA;

		xHigherPriorityTaskWoken = pdFALSE; 
		xQueueSendToBackFromISR(xMBReqQueue, &mb_event, &xHigherPriorityTaskWoken);

}

/**
 * This is modbus master execute function error process callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 * @param ucDestAddress destination salve address
 * @param pucPDUData PDU buffer data
 * @param ucPDULength PDU buffer length
 *
 */
void vMBMasterErrorCBExecuteFunction(UCHAR ucDestAddress, const UCHAR* pucPDUData,
        USHORT ucPDULength) 
{
		mb_event = EV_MASTER_ERROR_EXECUTE_FUNCTION;

		xHigherPriorityTaskWoken = pdFALSE; 
		xQueueSendToBackFromISR(xMBReqQueue, &mb_event, &xHigherPriorityTaskWoken);

}

/**
 * This is modbus master request process success callback function.
 * @note There functions will block modbus master poll while execute OS waiting.
 * So,for real-time of system.Do not execute too much waiting process.
 *
 */
void vMBMasterCBRequestScuuess( void ) 
{
		mb_event = EV_MASTER_PROCESS_SUCESS;

		xHigherPriorityTaskWoken = pdFALSE; 
		xQueueSendToBackFromISR(xMBReqQueue, &mb_event, &xHigherPriorityTaskWoken);

}

/**
 * This function is wait for modbus master request finish and return result.
 * Waiting result include request process success, request respond timeout,
 * receive data error and execute function error.You can use the above callback function.
 * @note If you are use OS, you can use OS's event mechanism. Otherwise you have to run
 * much user custom delay for waiting.
 *
 * @return request error code
 */
eMBMasterReqErrCode eMBMasterWaitRequestFinish( void ) 
{    
		int eQueue;  
		eMBMasterReqErrCode    eErrStatus = MB_MRE_NO_ERR;
	
		if(xQueueReceive(xMBReqQueue, &eQueue, 5000/portTICK_RATE_MS) != pdPASS)
			return MB_MRE_TIMEDOUT;
		//printf("Rec xMBReqQueue: %d\n", eQueue);
		switch (eQueue)
    {
    case EV_MASTER_PROCESS_SUCESS:
        break;
    case EV_MASTER_ERROR_RESPOND_TIMEOUT:
    {
        eErrStatus = MB_MRE_TIMEDOUT;
        break;
    }
    case EV_MASTER_ERROR_RECEIVE_DATA:
    {
        eErrStatus = MB_MRE_REV_DATA;
        break;
    }
    case EV_MASTER_ERROR_EXECUTE_FUNCTION:
    {
        eErrStatus = MB_MRE_EXE_FUN;
        break;
    }
    }
		return eErrStatus;
}

#endif
