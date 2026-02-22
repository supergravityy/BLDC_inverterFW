#include "exti.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

static typExti_handle exti_hallU;
static typExti_handle exti_hallV;
static typExti_handle exti_hallW;

void exti_init(void)
{
    uint32_t temp_reg = 0;

    exti_hallU.gpioPort = HALL_U_Port;
    exti_hallU.gpioPin = 0;
    exti_hallV.gpioPort = HALL_V_Port;
    exti_hallV.gpioPin = 1;
    exti_hallW.gpioPort = HALL_W_Port;
    exti_hallW.gpioPin = 2;
    exti_hallU.irqNo = EXTI_LINE_U_ISR;
    exti_hallV.irqNo = EXTI_LINE_V_ISR;
    exti_hallW.irqNo = EXTI_LINE_W_ISR;

    // 1. GPIO 핀을 입력으로 설정 (풀업)
    gpio_set_input(exti_hallU.gpioPort, exti_hallU.gpioPin, GPIO_PULL_UP);
    gpio_set_input(exti_hallV.gpioPort, exti_hallV.gpioPin, GPIO_PULL_UP);
    gpio_set_input(exti_hallW.gpioPort, exti_hallW.gpioPin, GPIO_PULL_UP);
    
    // 2. SYSCFG 설정: 포트 연결 
    SYSCFG->EXTICR[0] &= ~(SYSCFG_EXTICR1_EXTI0_Msk | SYSCFG_EXTICR1_EXTI1_Msk | SYSCFG_EXTICR1_EXTI2_Msk);
    SYSCFG->EXTICR[0] |= (SYSCFG_EXTICR1_EXTI0_PD | SYSCFG_EXTICR1_EXTI1_PD | SYSCFG_EXTICR1_EXTI2_PD);

    // 3. EXTI 설정:
    temp_reg = UTILS_BIT_SHIFT(exti_hallU.gpioPin, 1) | UTILS_BIT_SHIFT(exti_hallV.gpioPin, 1) | UTILS_BIT_SHIFT(exti_hallW.gpioPin, 1);
    
    EXTI->IMR |= temp_reg; // 인터럽트 요청 활성화
    EXTI->RTSR |= temp_reg; // 상승 에지 트리거 설정
    EXTI->FTSR |= temp_reg; // 하강 에지 트리거 설정

    // 4. NVIC 설정
    NVIC_EnableIRQ(exti_hallU.irqNo);
    NVIC_EnableIRQ(exti_hallV.irqNo);
    NVIC_EnableIRQ(exti_hallW.irqNo);
}

inline bool exti_check_extiPR(uint8_t extiNum)
{
    switch(extiNum)
    {
        case 0:
            return (EXTI->PR & EXTI_PR_PR0) != 0;
        case 1:
            return (EXTI->PR & EXTI_PR_PR1) != 0;
        case 2:
            return (EXTI->PR & EXTI_PR_PR2) != 0;
        default:
            return false;
    }
}

inline void exti_clear_extiPR(uint8_t extiNum)
{
    switch(extiNum)
    {
        case 0:
            EXTI->PR |= EXTI_PR_PR0;
            break;
        case 1:
            EXTI->PR |= EXTI_PR_PR1;
            break;
        case 2:
            EXTI->PR |= EXTI_PR_PR2;
            break;
        default:
            break;
    }
}