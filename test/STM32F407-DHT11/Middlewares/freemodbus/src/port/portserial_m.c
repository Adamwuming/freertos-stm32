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
 * File: $Id: portserial_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions $
 */
/* ----------------------- System includes ----------------------------------*/
#include <string.h>

/* ----------------------- Platform includes --------------------------------*/
#include "stm32f4xx_hal.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#include "port.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Static variables ---------------------------------*/

/* ----------------------- Defines ------------------------------------------*/
/* serial transmit event */
#define EVENT_SERIAL_TRANS_START    (1<<0)

extern UART_HandleTypeDef huart1;
/* ----------------------- static functions ---------------------------------*/

/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortSerialInit(ULONG ulBaudRate, UCHAR ucDataBits,
        eMBParity eParity)
{
		/*
		MX_USART1_UART_Init();
		*/
    return TRUE;
}

void vMBMasterPortSerialEnable(BOOL xRxEnable, BOOL xTxEnable)
{
    if(TRUE==xRxEnable)
    {
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
    }
    else
    {
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_RXNE);
    }
    
	if(TRUE==xTxEnable)
    {
        __HAL_UART_ENABLE_IT(&huart1, UART_IT_TXE);
    }
    else
    {
        __HAL_UART_DISABLE_IT(&huart1, UART_IT_TXE);
    }
}

void vMBMasterPortClose(void)
{
    
}

BOOL xMBMasterPortSerialPutByte(UCHAR ucByte)
{
  if(HAL_UART_Transmit(&huart1, &ucByte, 1, 3) == HAL_OK) return TRUE;
  else return FALSE;
}

BOOL xMBMasterPortSerialPutADU(uint8_t *pData, uint16_t Size)
{
	HAL_UART_Transmit_DMA(&huart1, pData, Size);
  return TRUE;
}

BOOL xMBMasterPortSerialGetByte(UCHAR * pucByte)
{
  if(HAL_UART_Receive(&huart1, pucByte, 1, 3) == HAL_OK)  return TRUE;
  else return FALSE;
}

/* 
 * Create an interrupt handler for the transmit buffer empty interrupt
 * (or an equivalent) for your target processor. This function should then
 * call pxMBFrameCBTransmitterEmpty( ) which tells the protocol stack that
 * a new character can be sent. The protocol stack will then call 
 * xMBPortSerialPutByte( ) to send the character.
 */
void prvvUARTTxReadyISR(void)
{
    pxMBMasterFrameCBTransmitterEmpty();
}

/* 
 * Create an interrupt handler for the receive interrupt for your target
 * processor. This function should then call pxMBFrameCBByteReceived( ). The
 * protocol stack will then call xMBPortSerialGetByte( ) to retrieve the
 * character.
 */
void prvvUARTRxISR(void)
{
    pxMBMasterFrameCBByteReceived();
}

/**
 * Software simulation serial transmit IRQ handler.
 *
 * @param parameter parameter
 */
//static void serial_soft_trans_irq(void* parameter) {
//    rt_uint32_t recved_event;
//    while (1)
//    {
//        /* waiting for serial transmit start */
//        rt_event_recv(&event_serial, EVENT_SERIAL_TRANS_START, RT_EVENT_FLAG_OR,
//                RT_WAITING_FOREVER, &recved_event);
//        /* execute modbus callback */
//        prvvUARTTxReadyISR();
//    }
//}

/**
 * This function is serial receive callback function
 *
 * @param dev the device of serial
 * @param size the data size that receive
 *
 * @return return RT_EOK
 */
//static rt_err_t serial_rx_ind(rt_device_t dev, rt_size_t size) {
//    prvvUARTRxISR();
//    return RT_EOK;
//}

#endif
