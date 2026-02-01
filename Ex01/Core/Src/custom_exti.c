/*
 * custom_exti.c
 *
 *  Created on: Feb 1, 2026
 *      Author: Smersh
 */

#include "custom_exti.h"
#include "utils.h"
#include "custom_gpio.h"

// ─── GPIO 핀 정의 ───
#define GPIOC_SWT            (6U)   // PC6: LED
#define GPIOD_BTN            (4U)   // PD4: Button / EXTI4

// ─── RCC 클럭 매크로 ───
#define AHB1ENR_GPIOD_EN     (SETUP_SHIFT_WRITE_BIT(3, 1))
#define AHB1ENR_GPIOC_EN     (SETUP_SHIFT_WRITE_BIT(2, 1))
#define APB2ENR_SYSCFG_EN    (SETUP_SHIFT_WRITE_BIT(14, 1)) // EXTI 라인 매핑 -> EXTICR[0~3] 레지스터를 통해 어떤 GPIO가 EXTIx와 연결될지 설정

// ─── GPIO 설정 매크로 ───
#define GPIO_MODER_INPUT     (0x0U)
#define GPIO_MODER_OUTPUT    (0x1U)
#define GPIO_OSPEEDR_MID     (0x1U)

#define GPIOC_MODER_MASK     (SETUP_SHIFT_WRITE_BIT(GPIOC_SWT * 2, 0x3))
#define GPIOC_MODER_OUTPUT   (SETUP_SHIFT_WRITE_BIT(GPIOC_SWT * 2, GPIO_MODER_OUTPUT))
#define GPIOC_OSPEED_MASK    (SETUP_SHIFT_WRITE_BIT(GPIOC_SWT * 2, 0x3))
#define GPIOC_OSPEED_MED     (SETUP_SHIFT_WRITE_BIT(GPIOC_SWT * 2, GPIO_OSPEEDR_MID))

#define GPIOD_MODER_MASK     (SETUP_SHIFT_WRITE_BIT(GPIOD_BTN * 2, 0x3))

// ─── SYSCFG EXTI 설정 ───
#define SYSCFG_EXTICR1_EXTI4_MASK    (0xFU << 0)
#define SYSCFG_EXTICR1_EXTI4_PD      (0x3U << 0) // PD = 0011

// ─── EXTI 설정 ───
#define EXTI_LINE_4          (SETUP_SHIFT_WRITE_BIT(GPIOD_BTN,1))

void setUp_exti(void)
{
    /* 1. GPIOD, GPIOC 클럭 ON */
    RCC->AHB1ENR |= AHB1ENR_GPIOD_EN | AHB1ENR_GPIOC_EN;

    /* 2. PC6 → Output 모드 설정 */
    GPIOC->MODER &= ~GPIOC_MODER_MASK;
    GPIOC->MODER |=  GPIOC_MODER_OUTPUT;

    /* 3. PC6 → 중간 속도 설정 */
    GPIOC->OSPEEDR &= ~GPIOC_OSPEED_MASK;
    GPIOC->OSPEEDR |=  GPIOC_OSPEED_MED;

    /* 4. PD4 → 입력 모드 설정 */
    GPIOD->MODER &= ~GPIOD_MODER_MASK;

    /* 5. SYSCFG 클럭 ON */
    RCC->APB2ENR |= APB2ENR_SYSCFG_EN;

    /* 6. EXTI4 입력 소스를 Port D로 설정 */
    SYSCFG->EXTICR[1] &= ~SYSCFG_EXTICR1_EXTI4_MASK; // Clear EXTI4 4bits
    SYSCFG->EXTICR[1] |=  SYSCFG_EXTICR1_EXTI4_PD;	// 0x3 = Port D

    /* 7. EXTI4 Falling-edge + 인터럽트 언마스크 */
    EXTI->IMR  |= EXTI_LINE_4; 	// 인터럽트 요청 언마스크
    EXTI->FTSR |= EXTI_LINE_4;	// falling 엣지 on
    EXTI->PR    = EXTI_LINE_4;	// pending bit 클리어
    // 펜딩 -> 인터럽트 요청이 들어왔는데 아직 처리 중이거나 처리 대기 중인 상태
    // 총 24개의 비트가 있는 이유 -> EXTI16 이상 (일부 내부 주변장치에 연결됨)

    /* 8. NVIC EXTI4 IRQ 활성화 */
    NVIC_EnableIRQ(EXTI4_IRQn);
}

void EXTI4_IRQHandler(void)
{
	if(EXTI->PR & EXTI_LINE_4)
	{
		EXTI->PR = EXTI_LINE_4; // 1을 써야 clr 됨
		control_GPIO_toggle(GPIOD,GPIOD_FLT_LED);
	}
}
