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

void setUp_adc(void);
float control_getADC1_val(float (*userConvFunc)(uint16_t));
float control_example_convADC_val(uint16_t rawVal);
uint16_t control_pollADC1_rawval(void);

#endif /* INC_CUSTOM_ADC_H_ */
