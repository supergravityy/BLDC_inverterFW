#pragma once

#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"

typedef enum {
    GPIO_INPUT_MODE     = 0x00,
    GPIO_OUTPUT_MODE    = 0x01,
    GPIO_AF_MODE        = 0x02,
    GPIO_ANALOG_MODE    = 0x03
} typGPIO_mode;

typedef enum {
    GPIO_SPD_LOW        = 0x00,
    GPIO_SPD_MEDIUM     = 0x01,
    GPIO_SPD_HIGH       = 0x02,
    GPIO_SPD_VERY_HIGH  = 0x03
} typGPIO_spd;

typedef enum {
    GPIO_PULL_NONE      = 0x00, // 플로팅 (No Pull-up, No Pull-down)
    GPIO_PULL_UP        = 0x01, // 내부 풀업 활성화
    GPIO_PULL_DOWN      = 0x02  // 내부 풀다운 활성화
} typGPIO_pull;

typedef enum {
    GPIO_OTYPE_PP       = 0x00, // 푸시-풀 (일반적인 출력)
    GPIO_OTYPE_OD       = 0x01  // 오픈-드레인 (I2C, 외부 풀업용)
} typGPIO_Otype;

void gpio_init(void);
uint8_t gpio_read_pin(GPIO_TypeDef* gpio, uint8_t pin);
void gpio_toggle_pin(GPIO_TypeDef* gpio, uint8_t pin);
void gpio_write_pin(GPIO_TypeDef* gpio, uint8_t pin, uint8_t val);

void gpio_set_alterFunc(GPIO_TypeDef* gpio, uint8_t pin, uint8_t afNo, typGPIO_spd spdType);
void gpio_set_analog(GPIO_TypeDef* gpio, uint8_t pin);
void gpio_set_input(GPIO_TypeDef* gpio, uint8_t pin, typGPIO_pull pullType);
void gpio_set_output(GPIO_TypeDef* gpio, uint8_t pin, typGPIO_Otype outType, 
    typGPIO_pull pullType, typGPIO_spd spdType);
