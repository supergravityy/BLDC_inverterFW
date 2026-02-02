/*
 * custom_adc.c
 *
 *  Created on: Feb 2, 2026
 *      Author: Smersh
 */

#include "custom_adc.h"
#include "custom_gpio.h"
#include "utils.h"

#define ADC_VREF 		(3.3f)
#define ADC_RESOL		(4095U)

volatile uint16_t adc_rawVal;
volatile float adc_throttleV;


void setUp_adc1(void)
{
	/* 1. GPIO 클럭 활성화 (GPIOA) */
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOA_EN;

	/* 2. PA7 = 아날로그 입력 (ADC1_IN7) */
	GPIOA->MODER |= SETUP_SHIFT_WRITE_BIT(2 * GPIOA_IN7, GPIO_MODE_ANALOG);

	/* 3. ADC1 클럭 & 공통 프리스케일 */
	RCC->APB2ENR |= RCC_APB2ENR_ADC1_EN; // RCC 설정에서 이미 54mhz로 설정함
	ADC->CCR |= SETUP_SHIFT_WRITE_BIT(16,0x02); // plck2 / 6 -> 9mhz

	/* 4. ADC1 단일-변환 설정 */
	ADC1->CR2 = ADC_CR2_AD_ON; // adc 시작
	ADC1->SMPR2 = SETUP_SHIFT_WRITE_BIT(3 * GPIOA_IN7 ,ADC_SMPR2_SAMPLE_TIME); // 샘플링 시간 조정
	ADC1->SQR1 &= ~(0xF << 20); // 딱 1번만 conv -> regular group mode
	ADC1->SQR3 |= GPIOA_IN7; // A7번핀이 주기적으로 변환할 핀
}

uint16_t control_pollADC1_rawval(void)
{
	uint16_t retVal;

	ADC1->CR2 |= ADC_CR2_SWSTART_EN; // 변환시작

	while((ADC1->SR & ADC_SR_EOC) == 0); // 변환 끝 대기

	retVal = ADC1->DR;

	return retVal;
}

float control_example_convADC_val(uint16_t rawVal)
{
	return rawVal * ADC_VREF / ((float)(ADC_RESOL));
}

float control_getADC1_val(float (*userConvFunc)(uint16_t))
{
	uint16_t retVal = control_pollADC1_rawval();

	return userConvFunc(retVal);
}
