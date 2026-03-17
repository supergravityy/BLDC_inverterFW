#include "uart.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

#define UART_STOPBIT_1        (0)
#define UART_STOPBIT_2        (2UL)

typUart_handle uart3_handler;
typUart_handle uart2_handler;

#define UART_AT09_HANDLER      (USART3)
#define UART_DEBUG_HANDLER     (USART2)

static void uart_base_init(typUart_handle* hUart, uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity)
{
    hUart->baudrate = baudrate;
    hUart->dataBits = dataBits;
    hUart->stopBits = stopBits;
    hUart->parity = parity;

    // 1) USART 레지스터 초기화
    hUart->inst->CR1 = 0;
    hUart->inst->CR2 = 0;
    hUart->inst->CR3 = 0;
    
    // 2) 보드레이트 설정 (BRR = PCLK / Baud)
    hUart->inst->BRR = UART_PCLK_FREQ_HZ / baudrate;

    // 3) 패리티 비트 설정
    if(parity != UART_PARITY_NONE) 
    {
        hUart->inst->CR1 |= USART_CR1_PCE;
        if(parity == UART_PARITY_ODD) hUart->inst->CR1 |= USART_CR1_PS;
        else                          hUart->inst->CR1 &= ~USART_CR1_PS;
    }

    // 4) 스탑 비트 설정
    hUart->inst->CR2 &= ~USART_CR2_STOP_Msk;
    if(stopBits == 2) 
        hUart->inst->CR2 |= UTILS_BIT_SHIFT(USART_CR2_STOP_Pos, UART_STOPBIT_2);
    else 
        hUart->inst->CR2 |= UTILS_BIT_SHIFT(USART_CR2_STOP_Pos, UART_STOPBIT_1);

    // 5) TE, RE, UE 활성화
    hUart->inst->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE);

    hUart->is_initialized = true;
}

// UART3 (Bluetooth - AT09) 초기화
void uart_AT09_init(uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity)
{
    uart3_handler.inst = UART_AT09_HANDLER; // USART3

    // UART3 전용 핀 설정
    gpio_set_alterFunc(BLUETOOTH_TX_Port, 8, 7, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(BLUETOOTH_RX_Port, 9, 7, GPIO_SPD_VERY_HIGH);

    // 공용 로직 호출
    uart_base_init(&uart3_handler, baudrate, dataBits, stopBits, parity);
}

// UART2 (Debug) 초기화
void uart_debug_init(uint32_t baudrate, uint8_t dataBits, uint8_t stopBits, typUart_parity parity)
{
    uart2_handler.inst = USART2;

    // UART2 전용 핀 설정
    gpio_set_alterFunc(USART2_TX_Port, 5, 7, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(USART2_RX_Port, 6, 7, GPIO_SPD_VERY_HIGH);

    // 공용 로직 호출
    uart_base_init(&uart2_handler, baudrate, dataBits, stopBits, parity);
}

static inline void uart_sendChar_polling(typUart_handle* hUart, char c)
{
    // TXE 플래그가 설정될 때까지 대기
    while(!(hUart->inst->ISR & USART_ISR_TXE));
    
    // 데이터 전송
    hUart->inst->TDR = (uint8_t)c;
}

static inline void uart_sendStr_polling(typUart_handle* hUart, char* str, uint32_t len)
{
    if(str == NULL || len == 0) return;

    for(uint32_t i = 0; i < len; i++)
    {
        uart_sendChar_polling(hUart, str[i]);
    }
}

static inline void uart_send_uint(typUart_handle* hUart, uint32_t val)
{
    char tempBuff[11]; // 32비트 최대값 4,294,967,295 (10자리) + 여유분
    uint8_t idx = 0;

    if (val == 0)
    {
        uart_sendChar_polling(hUart, '0');
        return;
    }

    // 1. 역순으로 버퍼에 저장
    while (val > 0 && idx < 10)
    {
        tempBuff[idx++] = (char)((val % 10) + '0');
        val /= 10;
    }

    // 2. 버퍼를 역순으로 읽어 출력
    while (idx > 0)
    {
        uart_sendChar_polling(hUart, tempBuff[--idx]);
    }
}

static inline void uart_send_int(typUart_handle* hUart, int32_t val)
{
    if(val < 0) {
        uart_sendChar_polling(hUart, '-');
        val = -val;
    }
    uart_send_uint(hUart, (uint32_t)val);
}

static inline void uart_sendFloat(typUart_handle* hUart, float val, uint8_t decimals)
{
    uint32_t intPart = 0;
    float fracPart = 0.0f;

    // 1. 부호처리
    if(val < 0)
    {
        uart_sendChar_polling(hUart, '-');
        val = -val; // 양수로 변환
    }

    // 2. 정수부 처리
    intPart = (uint32_t)val;
    fracPart = val - (float)intPart;
    uart_send_uint(hUart, intPart);

    if(decimals > 0)
    {
        uart_sendChar_polling(hUart, '.');

        // 3. 소수부 처리
        for(uint8_t i = 0; i < decimals; i++)
        {
            fracPart *= 10.0f;
            uint8_t digit = (uint8_t)fracPart;
            uart_sendChar_polling(hUart, (char)(digit + '0'));
            fracPart -= (float)digit; // 소수부에서 해당 자리수 제거
        }
    }
}

void uart_AT09_sendStr_polling(char* str, uint32_t len)
{
    uart_sendStr_polling(&uart3_handler, str, len);
}

void uart_AT09_sendInteger_polling(int32_t val)
{
    uart_send_int(&uart3_handler, val);
}

void uart_AT09_sendFloat_polling(float val, uint8_t decimals)
{
    uart_sendFloat(&uart3_handler, val, decimals);
}

void uart_debug_sendStr_polling(char* str, uint32_t len)
{
    uart_sendStr_polling(&uart2_handler, str, len);
}

void uart_debug_sendInt_polling(int val)
{
	uart_send_int(&uart2_handler, val);
}

void uart_debug_sendFloat_polling(float val, uint8_t decimals)
{
    uart_sendFloat(&uart2_handler, val, decimals);
}
