/*
 * custom_tim.h
 *
 *  Created on: Feb 2, 2026
 *      Author: Smersh
 */

#ifndef INC_CUSTOM_TIM_H_
#define INC_CUSTOM_TIM_H_

#include "stm32f767xx.h"
#include "utils.h"

#define GPIOC_PWM_PINNUM	(6U)

#define RCC_AHB1ENR_GPIOC_EN	(SETUP_SHIFT_WRITE_BIT(2,1))
#define RCC_AHB1ENR_GPIOD_EN	(SETUP_SHIFT_WRITE_BIT(3,1))
#define RCC_APB2ENR_TIM1_EN		(SETUP_SHIFT_WRITE_BIT(0,1))

#define TIM1_EGR_UG_EN			(SETUP_SHIFT_WRITE_BIT(0,1))
#define TIM1_DIER_UIE_ON		(SETUP_SHIFT_WRITE_BIT(0,1))
#define TIM1_CR1_CEN_ON			(SETUP_SHIFT_WRITE_BIT(0,1))
#define TIM1_SR_UIF_FLG			(SETUP_SHIFT_WRITE_BIT(0,1))

void setUp_tim1(void);

#endif /* INC_CUSTOM_TIM_H_ */
