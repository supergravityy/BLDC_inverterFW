#include "dac.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

typDac_handle vDac_Handler;

void dac_init(void)
{
    vDac_Handler.inst = DAC;

    gpio_set_analog(DAC_OUT1_Port,4);
    gpio_set_analog(DAC_OUT2_Port,5);

    // OP-amp ON(BOFF) + 트리거 금지(코어가 쓰자마자 바로 출력)
    DAC->CR &= ~((DAC_CR_BOFF1 | DAC_CR_TEN1) | (DAC_CR_BOFF2 | DAC_CR_TEN2));
    // DAC on
    DAC->CR |= DAC_CR_EN1 | DAC_CR_EN2;

    vDac_Handler.is_initialized_ch1 = ((DAC->CR & DAC_CR_EN1) == 1);
    vDac_Handler.is_initialized_ch2 = ((DAC->CR & DAC_CR_EN2) == 1);
}

void DAC_setValue_ch1(uint16_t val) 
{
    // 12bit 오른쪽 정렬
    DAC->DHR12R1 = val & 0x0FFF;
}

void DAC_setValue_ch2(uint16_t val)
{
    DAC->DHR12R2 = val & 0x0FFF;
}