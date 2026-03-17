#include "uart.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

#define UART_STOPBIT_1          (0)
#define UART_STOPBIT_2          (2UL)

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
    hUart->inst->CR1 |= (USART_CR1_TE | USART_CR1_RE | USART_CR1_UE | USART_CR1_RXNEIE);

    hUart->is_initialized = true;
}

static void uart_rxbuff_clrBuff(typUart_rxBuff* buff)
{
    for(uint16_t i = 0; i < UART_RX_RING_BUFF_SIZE; i++)  
        buff->buffer[i] = 0x0;

    buff->setIdx = 0;
    buff->getIdx = 0;
    buff->msg_rdy = false;
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
    uart_rxbuff_clrBuff(&uart3_handler.rxBuff);
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
    uart_rxbuff_clrBuff(&uart2_handler.rxBuff);
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

inline static void uart_recv_ISR_handler(typUart_handle* huart)
{
    char rxData = 0;

    // 1. 오버런 에러(ORE)가 발생했는지 확인
    if ((huart->inst->ISR & USART_ISR_ORE) != 0)
    {
        // ICR(Interrupt Clear Register)의 ORECF 비트를 세팅하여 에러 플래그 강제 클리어
        // ORE가 안꺼지고 RXNE만 찾으면, ISR 무한루프에 걸릴 수 있음
        huart->inst->ICR |= USART_ICR_ORECF; 
    }

    // 2. 정상적인 데이터 수신(RXNE) 확인
    if ((huart->inst->ISR & USART_ISR_RXNE) != 0)
    {
        rxData = (char)(huart->inst->RDR & 0xFF); 

        uart_sendChar_polling(huart,rxData);
        if(rxData == '\r') uart_sendChar_polling(huart,'\n');

        // 엔터키(\n, \r)는 버퍼에 넣지 않고 플래그만 세팅
        if (rxData == '\n' || rxData == '\r')
        {
            huart->rxBuff.msg_rdy = true; 
        }
        else
        {
            // 일반 문자는 버퍼에 저장하고 setIdx 전진
            huart->rxBuff.buffer[huart->rxBuff.setIdx] = rxData;
            huart->rxBuff.setIdx = (huart->rxBuff.setIdx + 1) % UART_RX_RING_BUFF_SIZE;
        }
    }
}

static inline bool uart_recvExtract_string(typUart_handle* huart, char* retBuff, uint16_t* strSize, uint16_t buffSize)
{
    char ch = 0;
    uint16_t rxSize = 0;

    // 1. 포인터 및 사이즈 방어 코드
    if(retBuff == NULL || buffSize == 0 || strSize == NULL)
        return false;
    
    // 2. 가져올 메시지가 없다면 false를 반환하여 상위 태스크가 무시하도록 함
    if(huart->rxBuff.msg_rdy == false)
    {
        return false;
    }
    else
    {
        huart->rxBuff.msg_rdy = false;
        
        // 3. 하나씩 가져오기
        while( (huart->rxBuff.getIdx != huart->rxBuff.setIdx) && (rxSize < buffSize - 1) )
        {
            ch = huart->rxBuff.buffer[huart->rxBuff.getIdx];
            huart->rxBuff.getIdx = (huart->rxBuff.getIdx + 1) % UART_RX_RING_BUFF_SIZE;
            retBuff[rxSize++] = ch;
        }

        retBuff[rxSize] = '\0';
        *strSize = rxSize;
        
        // 메시지를 성공적으로 추출했으므로 true 반환
        return true; 
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

void uart_debug_enable_rxInterrupt(void) // 상위 계층의 초기화가 전부 이루어지면 호출해서 start
{
    NVIC_SetPriority(USART2_IRQn, 3); // 우선순위를 모터 제어 타이머보다 낮게 설정
    NVIC_EnableIRQ(USART2_IRQn);
}

bool uart_debug_recvExtract_string(char* retBuff, uint16_t* strSize, uint16_t buffSize)
{
    return uart_recvExtract_string(&uart2_handler, retBuff, strSize, buffSize);
}

void USART2_IRQHandler(void)
{
    uart_recv_ISR_handler(&uart2_handler);
}
