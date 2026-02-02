/*
 * custom_adc.h
 *
 *  Created on: Feb 2, 2026
 *      Author: Smersh
 */

#ifndef INC_CUSTOM_ADC_H_
#define INC_CUSTOM_ADC_H_

#include "stm32f767xx.h"
#include "utils.h"

#define GPIOA_IN7	(7U)
#define RCC_AHB1ENR_GPIOA_EN		(SETUP_SHIFT_WRITE_BIT(0,1))
#define RCC_APB2ENR_ADC1_EN			(SETUP_SHIFT_WRITE_BIT(8,1))

#define ADC_CR2_AD_ON				(SETUP_SHIFT_WRITE_BIT(0,1))
#define ADC_SMPR2_SAMPLE_TIME		(0x03) // 56cycle -> 6.2 us @ 9 MHz
// #define ADC_SQR1_LEN_CLR_MASK		(uint8_t)~(0x0f)

#define ADC_CR2_SWSTART_EN			(SETUP_SHIFT_WRITE_BIT(30,1))
#define ADC_SR_EOC_EN				(SETUP_SHIFT_WRITE_BIT(1,1))


void setUp_adc1(void);
float control_getADC1_val(float (*userConvFunc)(uint16_t));
float control_example_convADC_val(uint16_t rawVal);
uint16_t control_pollADC1_rawval(void);

#endif /* INC_CUSTOM_ADC_H_ */
