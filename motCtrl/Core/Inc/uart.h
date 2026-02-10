#pragma once

#include "stm32f767xx.h"
#include "utils.h"

#define UART_PCLK_FREQ_HZ           (54000000UL)  // APB1 버스 클럭 주파수 (50 MHz)
#define UART_RCV_TIMEOUT_TICK       (50000)

#pragma pack(push,1)

typedef enum uart_parity
{
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
}typUart_parity;

typedef struct uart_handle
{
    USART_TypeDef* inst;

    uint32_t baudrate;
    uint8_t dataBits;
    uint8_t stopBits;
    typUart_parity parity;

    uint32_t rx_timeout_Tick;

    bool is_initialized;
}typUart_handle;

#pragma pack(pop)

void uart_AT09_sendStr_polling(char* str, uint32_t len);
void uart_AT09_recvStr_polling(char* buff, uint32_t len);
void uart_AT09_sendFloat_polling(float val, uint8_t decimals);
void uart_debug_reportSpd_polling(float rpm);
void uart_debug_sendStr_polling(char* str, uint32_t len);
void uart_debug_sendFloat_polling(float val, uint8_t decimals);

void uart_AT09_init(uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity);
void uart_debug_init(uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity);