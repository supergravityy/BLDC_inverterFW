#pragma once

#include "stm32f767xx.h"
#include "utils.h"

#define EXTI_LINE_U_ISR   EXTI0_IRQn
#define EXTI_LINE_V_ISR   EXTI1_IRQn
#define EXTI_LINE_W_ISR   EXTI2_IRQn

#pragma pack(push, 1)

typedef struct exti_handle
{
    GPIO_TypeDef*   gpioPort; // EXTI 핀의 포트
    uint8_t         gpioPin;  // EXTI 핀 번호 (0~15)
    IRQn_Type       irqNo;       // 해당 EXTI 핀에 대응하는 IRQ 번호

    bool is_initialized;
}typExti_handle;

#pragma pack(pop)

void exti_init(void);
bool exti_check_extiPR(uint8_t extiNum);
void exti_clear_extiPR(uint8_t extiNum);