/*
 * custom_gpio.c
 *
 *  Created on: Jan 31, 2026
 *      Author: Smersh
 */

#include "custom_gpio.h"
#include "utils.h"

void setUp_GPIO(void)
{
	GPIOD->MODER &= ~(SETUP_SHIFT_WRITE_BIT(GPIOD_FLT_LED * 2, 0x03));
	GPIOD->OSPEEDR &=  ~(SETUP_SHIFT_WRITE_BIT(GPIOD_FLT_LED * 2, 0x03));

	GPIOD->MODER |= SETUP_SHIFT_WRITE_BIT(GPIOD_FLT_LED * 2,GPIO_MODE_OUTPUT); // gpiod (MODER3)
	GPIOD->OSPEEDR |= SETUP_SHIFT_WRITE_BIT(GPIOD_FLT_LED * 2,GPIO_OUTSPD_MID); // gpiod (OSPEEDR3)

	GPIOD->MODER &= ~(SETUP_SHIFT_WRITE_BIT(GPIOD_SWT_BTTN * 2, 0x03));
	GPIOD->PUPDR &= ~(SETUP_SHIFT_WRITE_BIT(GPIOD_SWT_BTTN * 2, 0x03));

	GPIOD->MODER |= SETUP_SHIFT_WRITE_BIT(GPIOD_SWT_BTTN * 2,GPIO_MODE_INPUT);
	GPIOD->PUPDR &= ~(SETUP_SHIFT_WRITE_BIT(GPIOD_SWT_BTTN * 2, GPIO_PULL_UP));

}

void control_GPIO_write(GPIO_TypeDef * GPIOx, uint8_t pinNum, uint8_t Value)
{
	if(Value == 0x01)
		GPIOx->ODR |= SETUP_SHIFT_WRITE_BIT(pinNum,1);
	else
		GPIOx->ODR &= SETUP_SHIFT_WRITE_BIT(pinNum,0);
}

void control_GPIO_toggle(GPIO_TypeDef * GPIOx, uint8_t pinNum)
{
	GPIOx->ODR ^= SETUP_SHIFT_WRITE_BIT(pinNum,1);
}

uint8_t control_GPIO_read(GPIO_TypeDef * GPIOx, uint8_t pinNum)
{
	return (uint8_t)(GPIOx->IDR & SETUP_SHIFT_WRITE_BIT(pinNum,1));
}
