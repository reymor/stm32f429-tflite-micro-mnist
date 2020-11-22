/*
 * debug.c
 *
 *  Created on: 14-Nov-2020
 *      Author: reymor
 */


/*---------------------------------------------------------------------*
 *  include files                                                      *
 *---------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "main.h"

#include "debug.h"

extern void Error_Handler(void);

/*---------------------------------------------------------------------*
 *  local definitions                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  external declarations                                              *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  public data                                                        *
 *---------------------------------------------------------------------*/

UART_HandleTypeDef uart_debug_handle;



/*---------------------------------------------------------------------*
 *  private data                                                       *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  private functions                                                  *
 *---------------------------------------------------------------------*/
static void HAL_UART_Msp_Init(void);
/*---------------------------------------------------------------------*
 *  public functions                                                   *
 *---------------------------------------------------------------------*/

void debug_init (void)
{

	HAL_UART_Msp_Init();

	uart_debug_handle.Instance = DEBUGx;

	uart_debug_handle.Init.BaudRate = DEBUG_BAUDRATE;

	uart_debug_handle.Init.WordLength   = UART_WORDLENGTH_8B;
	uart_debug_handle.Init.StopBits     = UART_STOPBITS_1;
	uart_debug_handle.Init.Parity       = UART_PARITY_NONE;
	uart_debug_handle.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
	uart_debug_handle.Init.Mode         = UART_MODE_TX_RX;
	uart_debug_handle.Init.OverSampling = UART_OVERSAMPLING_16;

	if(HAL_UART_Init(&uart_debug_handle) != HAL_OK)
	{
		Error_Handler();
	}

}

static void HAL_UART_Msp_Init(void)
{
	GPIO_InitTypeDef  GPIO_InitStruct;

	/*##-1- Enable peripherals and GPIO Clocks #################################*/
	/* Enable GPIO TX/RX clock */
	DEBUGx_TX_GPIO_CLK_ENABLE();
	DEGUBx_RX_GPIO_CLK_ENABLE();
	/* Enable DEBUGx clock */
	DEBUGx_CLK_ENABLE();

	/*##-2- Configure peripheral GPIO ##########################################*/
	/* UART TX GPIO pin configuration  */
	GPIO_InitStruct.Pin       = DEBUGx_TX_PIN;
	GPIO_InitStruct.Mode      = GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull      = GPIO_NOPULL;
	GPIO_InitStruct.Speed     = GPIO_SPEED_FAST;
	GPIO_InitStruct.Alternate = DEBUGx_TX_AF;

	HAL_GPIO_Init(DEBUGx_TX_GPIO_PORT, &GPIO_InitStruct);

	/* UART RX GPIO pin configuration  */
	GPIO_InitStruct.Pin = DEBUGx_RX_PIN;
	GPIO_InitStruct.Alternate = DEBUGx_RX_AF;

	HAL_GPIO_Init(DEBUGx_RX_GPIO_PORT, &GPIO_InitStruct);

	/*##-3- Configure the NVIC for UART ########################################*/
	/* NVIC for DEBUGUART */
	HAL_NVIC_SetPriority(DEBUGx_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(DEBUGx_IRQn);


}


void debug_chr (char chr)
{

	if(HAL_UART_Transmit_IT(&uart_debug_handle, (uint8_t*)&chr, 1) != HAL_OK)
	{
		Error_Handler();
	}

	while (HAL_UART_GetState(&uart_debug_handle) == HAL_UART_STATE_BUSY_TX ||
	HAL_UART_GetState(&uart_debug_handle) == HAL_UART_STATE_BUSY_TX_RX); /* wait until the uart is ready */
}

void USART1_IRQHandler (void)
{

	HAL_UART_IRQHandler(&uart_debug_handle);

}

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/
