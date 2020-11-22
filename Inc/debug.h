/*
 * debug.h
 *
 *  Created on: 14-Nov-2020
 *      Author: reymor
 */


#ifndef DEBUG_H_
#define DEBUG_H_

/*---------------------------------------------------------------------*
 *  additional includes                                                *
 *---------------------------------------------------------------------*/
#ifdef __cplusplus
 extern "C" {
#endif
/*---------------------------------------------------------------------*
 *  global definitions                                                 *
 *---------------------------------------------------------------------*/
 /* Definitions for Debug Port */

#define DEBUGx_CLK_ENABLE()              __HAL_RCC_USART1_CLK_ENABLE();
#define DEGUBx_RX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()
#define DEBUGx_TX_GPIO_CLK_ENABLE()      __HAL_RCC_GPIOA_CLK_ENABLE()

 #define DEBUGx                           USART1
 #define DEBUG_BAUDRATE					  9600
 #define DEBUGx_TX_PIN                    GPIO_PIN_9
 #define DEBUGx_TX_GPIO_PORT              GPIOA

 #define DEBUGx_TX_AF                     GPIO_AF7_USART1

 #define DEBUGx_RX_PIN                    GPIO_PIN_10
 #define DEBUGx_RX_GPIO_PORT              GPIOA

 #define DEBUGx_RX_AF                     GPIO_AF7_USART1

 /* Definition for USARTx's NVIC */
 #define DEBUGx_IRQn                      USART1_IRQn
 #define DEBUGx_IRQHandler                USART1_IRQHandler

#define DEBUGx_TX_AF                     GPIO_AF7_USART1

#define DEBUGx_RX_AF                     GPIO_AF7_USART1

/*---------------------------------------------------------------------*
 *  type declarations                                                  *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  function prototypes                                                *
 *---------------------------------------------------------------------*/

void debug_init (void);

void debug_chr (char chr);

/*---------------------------------------------------------------------*
 *  global data                                                        *
 *---------------------------------------------------------------------*/


/*---------------------------------------------------------------------*
 *  inline functions and function-like macros                          *
 *---------------------------------------------------------------------*/

/*---------------------------------------------------------------------*
 *  eof                                                                *
 *---------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

#endif /* DEBUG_H_ */
