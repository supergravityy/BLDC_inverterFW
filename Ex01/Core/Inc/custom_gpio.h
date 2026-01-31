/*
 * cusom_gpio.h
 *
 *  Created on: Jan 31, 2026
 *      Author: Smersh
 */

#ifndef INC_CUSTOM_GPIO_H_
#define INC_CUSTOM_GPIO_H_

#include "stm32f767xx.h"

#define GPIOD_FLT_LED			(3U)
#define GPIOD_SWT_BTTN			(4U)

#define GPIOD_BASE_ADDR			(0x40020C00UL)

#define GPIO_MODER_OFFSET		(0X00UL)
#define GPIO_OTYPER_OFFSET		(0X04UL)
#define GPIO_OSPEEDR_OFFSET		(0X08UL)
#define GPIO_PUPDR_OFFSET		(0X0CUL)
#define GPIO_IDR_OFFSET			(0X10UL)
#define GPIO_ODR_OFFSET			(0X14UL)

#define GPIOD_MODER				(*((volatile uint32_t *)(GPIOD_BASE_ADDR + GPIO_MODER_OFFSET)))
#define GPIOD_OTYPER			(*((volatile uint32_t *)(GPIOD_BASE_ADDR + GPIO_OTYPER_OFFSET)))
#define GPIOD_OSPEEDR			(*((volatile uint32_t *)(GPIOD_BASE_ADDR + GPIO_OSPEEDR_OFFSET)))
#define GPIOD_PUPDR				(*((volatile uint32_t *)(GPIOD_BASE_ADDR + GPIO_PUPDR_OFFSET)))
#define GPIOD_IDR				(*((volatile uint32_t *)(GPIOD_BASE_ADDR + GPIO_IDR_OFFSET)))
#define GPIOD_ODR				(*((volatile uint32_t *)(GPIOD_BASE_ADDR + GPIO_ODR_OFFSET)))

#define GPIO_MODE_INPUT			(0U)
#define GPIO_MODE_OUTPUT		(1U)
#define GPIO_MODE_ALTER_FUNC	(2U)
#define GPIO_MODE_ANALOG		(3U)

#define GPIO_OUTSPD_LOW			(0U)
#define GPIO_OUTSPD_MID			(1U)
#define GPIO_OUTSPD_HIGH		(2U)
#define GPIO_OUTSPD_VERY_HIGH	(3U)

#define GPIO_PULL_NOTHING		(0U)
#define GPIO_PULL_UP			(1U)
#define GPIO_PULL_DOWN			(2U)


void setUp_GPIO(void);
void control_GPIO_write(GPIO_TypeDef * GPIOx, uint8_t pinNum, uint8_t Value);
uint8_t control_GPIO_read(GPIO_TypeDef * GPIOx, uint8_t pinNum);
void control_GPIO_toggle(GPIO_TypeDef * GPIOx, uint8_t pinNum);

#endif /* INC_CUSTOM_GPIO_H_ */
