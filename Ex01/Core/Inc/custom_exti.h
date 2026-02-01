/*
 * custom_exti.h
 *
 *  Created on: Feb 1, 2026
 *      Author: Smersh
 */

#ifndef INC_CUSTOM_EXTI_H_
#define INC_CUSTOM_EXTI_H_

#include "stm32f767xx.h"
#include "utils.h"

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

void setUp_exti(void);

#endif /* INC_CUSTOM_EXTI_H_ */
