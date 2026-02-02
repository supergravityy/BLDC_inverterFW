/*
 * custom_uart.h
 *
 *  Created on: Feb 2, 2026
 *      Author: Smersh
 */

#ifndef INC_CUSTOM_UART_H_
#define INC_CUSTOM_UART_H_

#include "stm32f767xx.h"
#include "utils.h"

#define GPIOD_USART2_TX		(5U)
#define GPIOD_USART2_RX		(6U)

#define RCC_APB1ENR_USART2_EN			(SETUP_SHIFT_WRITE_BIT(17,1))
#define RCC_AHB1ENR_GPIOD_EN			(SETUP_SHIFT_WRITE_BIT(3,1))
#define GPIOD_AFR_AF7_USART2_TX			(0x07)
#define GPIOD_AFR_AF7_USART2_RX			(0x07)

#define USART_CR1_TX_EN					(SETUP_SHIFT_WRITE_BIT(3,1))
#define USART_CR1_RX_EN					(SETUP_SHIFT_WRITE_BIT(2,1))
#define USART_CR1_UART_EN				(SETUP_SHIFT_WRITE_BIT(0,1))
#define USART_ISR_TX_EMPTY				(SETUP_SHIFT_WRITE_BIT(7,1))
#define USART_ISR_RX_EMPTY				(SETUP_SHIFT_WRITE_BIT(5,1))

char control_UART2_receiveChar(void);
void control_UART2_sendChar(const char ch);
void setUp_uart2(void);

#endif /* INC_CUSTOM_UART_H_ */
