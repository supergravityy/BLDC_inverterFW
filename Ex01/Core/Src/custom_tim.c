/*
 * custom_tim.c
 *
 *  Created on: Feb 2, 2026
 *      Author: Smersh
 */

#include "custom_tim.h"
#include "utils.h"
#include "custom_gpio.h"


void setUp_tim1(void)
{
	/* 1. GPIOD,GPIOC 클록 활성화 */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOC_EN | RCC_AHB1ENR_GPIOD_EN;

	/* 2 . PC6을 General-purpose output 모드로 설정 */
	GPIOC->MODER &= ~0x00003000;  // clear bits 13:12
	GPIOC->MODER |= SETUP_SHIFT_WRITE_BIT(GPIOC_PWM_PINNUM * 2, GPIO_MODE_OUTPUT);
	GPIOC->OSPEEDR &= ~0x00003000;
	GPIOC->OSPEEDR |= SETUP_SHIFT_WRITE_BIT(GPIOC_PWM_PINNUM * 2, GPIO_OUTSPD_MID);

	/* 3. TIM1 클록 활성화 */
	RCC->APB2ENR |= RCC_APB2ENR_TIM1_EN;

	/* 4. TIM1 CONFIG */
	TIM1->PSC = 21600 - 1; 	// 프리스케일러
	TIM1->ARR = 10000 - 1; 	// 맥시멈 카운터
	TIM1->EGR = TIM1_EGR_UG_EN; // tim1 카운터 초기화 -> 이후 동작하면서 hw에서 자동으로 초기화 됨

	/* 5. TIM1 인터럽트 허용 */
	TIM1->DIER |= TIM1_DIER_UIE_ON;
	NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);   // CMSIS 함수

	/* 6. 카운트 시작 */
	TIM1->CR1 |= TIM1_CR1_CEN_ON; // counter enable
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    if ((TIM1->SR & TIM1_SR_UIF_FLG) == 1) // TIM1의 플래그인지 확인
    {
        TIM1->SR &= ~TIM1_SR_UIF_FLG;       // TIM1의 UIF 클리어
        control_GPIO_toggle(GPIOC,GPIOC_PWM_PINNUM); // 1sec 마다 토글
    }
}
