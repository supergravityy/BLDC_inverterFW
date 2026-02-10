#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

void gpio_set_output(GPIO_TypeDef* gpio, uint8_t pin, typGPIO_Otype outType, 
    typGPIO_pull pullType, typGPIO_spd spdType)
{
    // 1. 모드 설정 (Output: 01)
    gpio->MODER &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->MODER |=  UTILS_BIT_SHIFT(pin * 2, 0x01);

    // 2. 출력 타입 설정
    gpio->OTYPER &= ~UTILS_BIT_SHIFT(pin, 0x01);
    gpio->OTYPER |=  UTILS_BIT_SHIFT(pin, outType);

    // 3. 속도 설정
    gpio->OSPEEDR &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->OSPEEDR |=  UTILS_BIT_SHIFT(pin * 2, spdType);

    // 4. Pull-up/down 설정
    gpio->PUPDR &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->PUPDR |=  UTILS_BIT_SHIFT(pin * 2, pullType);
}

void gpio_set_input(GPIO_TypeDef* gpio, uint8_t pin, typGPIO_pull pullType)
{
    // Mode: 00 (Input)
    gpio->MODER &= ~UTILS_BIT_SHIFT(pin * 2, 0x03); 
    
    // Pull 설정
    gpio->PUPDR &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->PUPDR |=  UTILS_BIT_SHIFT(pin * 2, pullType);
}

// 아날로그 모드 전용: 모든 디지털 회로가 꺼지므로 Pull 설정도 보통 NONE
void gpio_set_analog(GPIO_TypeDef* gpio, uint8_t pin)
{
    // Mode: 11 (Analog)
    gpio->MODER &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->MODER |= UTILS_BIT_SHIFT(pin * 2, 0x03);
    
    // Pull: 00 (Floating)
    gpio->PUPDR &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
}

void gpio_set_alterFunc(GPIO_TypeDef* gpio, uint8_t pin, uint8_t afNo, typGPIO_spd spdType)
{
    // 1. MODER를 Alternate Function 모드(10)로 설정
    gpio->MODER &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->MODER |=  UTILS_BIT_SHIFT(pin * 2, 0x02);
    
    // 2. AFR 레지스터 설정
    // pin / 8 은 인덱스(0 또는 1), pin % 8 은 해당 레지스터 내 위치
    // pin & 0x07 (=pin % 8) 은 두 레지스터에서 어떤 레지스터에 값을 쓸지를 정함
    // * 4 는 한칸이 4비트임
    gpio->AFR[pin >> 3] &= ~UTILS_BIT_SHIFT((pin & 0x07) * 4, 0x0F);
    gpio->AFR[pin >> 3] |= UTILS_BIT_SHIFT((pin & 0x07) * 4,afNo);

    // 3. 스피드 설정
    gpio->OSPEEDR &= ~UTILS_BIT_SHIFT(pin * 2, 0x03);
    gpio->OSPEEDR |=  UTILS_BIT_SHIFT(pin * 2, spdType);
}

void gpio_write_pin(GPIO_TypeDef* gpio, uint8_t pin, uint8_t val)
{
    if(val == 0)
    {
        // BSRR의 상위 16비트는 Reset(0)을 담당
        gpio->BSRR = UTILS_BIT_SHIFT(pin + 16, 1);
    }
    else
    {
        // BSRR의 하위 16비트는 Set(1)을 담당
        gpio->BSRR = UTILS_BIT_SHIFT(pin, 1);
    }
}

void gpio_toggle_pin(GPIO_TypeDef* gpio, uint8_t pin)
{
    gpio->ODR ^= UTILS_BIT_SHIFT(pin,1);
}

uint8_t gpio_read_pin(GPIO_TypeDef* gpio, uint8_t pin)
{
    return (uint8_t)((gpio->IDR & UTILS_BIT_SHIFT(pin, 1)) ? 1 : 0);
}

void gpio_init(void)
{
    gpio_set_output(GPIOC, 6, GPIO_OTYPE_PP, GPIO_PULL_UP, GPIO_SPD_LOW);
}