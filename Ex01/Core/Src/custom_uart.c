/*
 * custom_uart.c
 *
 *  Created on: Feb 2, 2026
 *      Author: Smersh
 */

#include "custom_uart.h"
#include "utils.h"
#include "custom_gpio.h"


void setUp_uart2(void)
{
	/* 1. USART2 및 관련 GPIO 클럭 활성화 */
	RCC->APB1ENR |= RCC_APB1ENR_USART2_EN;
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOD_EN;

	/* 2. PD5 (TX), PD6 (RX)를 Alternate Function (AF7) 모드로 설정 */
	GPIOD->MODER |= SETUP_SHIFT_WRITE_BIT(GPIOD_USART2_TX * 2, GPIO_MODE_ALTER_FUNC)
			| SETUP_SHIFT_WRITE_BIT(GPIOD_USART2_RX * 2, GPIO_MODE_ALTER_FUNC);

	/* 3. PD5, PD6에 대한 AF 설정 (AF7) */
	GPIOD->AFR[0] = SETUP_SHIFT_WRITE_BIT(GPIOD_USART2_TX * 2, GPIOD_AFR_AF7_USART2_TX)
			| SETUP_SHIFT_WRITE_BIT(GPIOD_USART2_RX * 2, GPIOD_AFR_AF7_USART2_RX);

	/* 4. USART2 설정: 비동기(8N1, 115200bps), TX/RX 활성 */
	USART2->CR1 = 0;    // 초기화 (비활성화)
	USART2->CR2 = 0;
	USART2->CR3 = 0;
	USART2->BRR = (uint32_t)(54000000.0f/9600.0f);

	/* 5. TX, RX 활성 및 USART2 활성화 */
	USART2->CR1 |= USART_CR1_TX_EN | USART_CR1_RX_EN | USART_CR1_UART_EN;
}

void control_UART2_sendChar(const char ch)
{
	while (!(USART2->ISR & USART_ISR_TX_EMPTY));
	// TXE (송신 데이터 레지스터 비어있음) 플래그 대기

	USART2->TDR = (uint8_t)ch;
}

char control_UART2_receiveChar(void)
{
	while (!(USART2->ISR & USART_ISR_RXNE));
	// RXNE (수신 데이터 레지스터에 데이터 있음) 플래그 대기

	return (char)(USART2->RDR & 0xFF);
}
