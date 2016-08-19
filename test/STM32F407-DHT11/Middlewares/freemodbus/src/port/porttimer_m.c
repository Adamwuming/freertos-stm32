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
 * File: $Id: porttimer_m.c,v 1.60 2013/08/13 15:07:05 Armink add Master Functions$
 */

/* ----------------------- Platform includes --------------------------------*/
#include "port.h"

/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mb_m.h"
#include "mbport.h"

#if MB_MASTER_RTU_ENABLED > 0 || MB_MASTER_ASCII_ENABLED > 0
/* ----------------------- Variables ----------------------------------------*/
extern TIM_HandleTypeDef htim4;

//static USHORT usT35TimeOut50us;
//static void timer_timeout_ind(void* parameter);

/* ----------------------- static functions ---------------------------------*/


/* ----------------------- Start implementation -----------------------------*/
BOOL xMBMasterPortTimersInit(USHORT usTimeOut50us)
{
    /* backup T35 ticks */
    //usT35TimeOut50us = usTimeOut50us;
		/*
		MX_TIM7_Init();
		*/

    return TRUE;
}

void vMBMasterPortTimersT35Enable()
{
    int tick_count = 40;	/*Down count value(50us), delay 2000us TIM_IT_UPDATE*/
	/* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_T35);

		__HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_SetCounter(&htim4, tick_count);
		__HAL_TIM_ENABLE(&htim4);
}

void vMBMasterPortTimersConvertDelayEnable()
{
    int tick_count = MB_MASTER_DELAY_MS_CONVERT;	/*Down count value(50us), delay 200ms TIM_IT_UPDATE*/
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_CONVERT_DELAY);

		__HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_SetCounter(&htim4, tick_count);
		__HAL_TIM_ENABLE(&htim4);
}

void vMBMasterPortTimersRespondTimeoutEnable()
{
    int tick_count = MB_MASTER_TIMEOUT_MS_RESPOND;	/*Down count value(50us), delay 2s TIM_IT_UPDATE*/
    /* Set current timer mode, don't change it.*/
    vMBMasterSetCurTimerMode(MB_TMODE_RESPOND_TIMEOUT);

		__HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_ENABLE_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_SetCounter(&htim4, tick_count);
		__HAL_TIM_ENABLE(&htim4);
}

void vMBMasterPortTimersDisable()
{
    __HAL_TIM_DISABLE(&htim4);
		__HAL_TIM_SetCounter(&htim4, 0xffff);
		__HAL_TIM_DISABLE_IT(&htim4, TIM_IT_UPDATE);
		__HAL_TIM_CLEAR_IT(&htim4, TIM_IT_UPDATE);
}

void prvvTIMERExpiredISR(void)
{
    (void) pxMBMasterPortCBTimerExpired();
}

//static void timer_timeout_ind(void* parameter)
//{
//    prvvTIMERExpiredISR();
//}

#endif
