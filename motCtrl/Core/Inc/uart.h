#pragma once

#include "stm32f767xx.h"
#include "utils.h"

#define UART_PCLK_FREQ_HZ           (54000000UL)  // APB1 버스 클럭 주파수 (50 MHz)
#define UART_RX_BUFF_SIZE           (64UL)

#pragma pack(push,1)

typedef enum uart_parity
{
    UART_PARITY_NONE = 0,
    UART_PARITY_EVEN,
    UART_PARITY_ODD
}typUart_parity;

typedef struct uart_rxHandle
{

}typUart_rxHandle;

typedef struct uart_handle
{
    USART_TypeDef* inst;

    uint32_t baudrate;
    uint8_t dataBits;
    uint8_t stopBits;
    typUart_parity parity;

    typUart_rxHandle rxhandle;

    bool is_initialized;
}typUart_handle;

#pragma pack(pop)

void uart_AT09_sendStr_polling(char* str, uint32_t len);
void uart_AT09_sendInteger_polling(int32_t val);
void uart_AT09_sendFloat_polling(float val, uint8_t decimals);
void uart_debug_sendStr_polling(char* str, uint32_t len);
void uart_debug_sendFloat_polling(float val, uint8_t decimals);
void uart_debug_sendInt_polling(int val);

void uart_AT09_init(uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity);
void uart_debug_init(uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity);
