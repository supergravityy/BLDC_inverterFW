#pragma once

#include "stm32f767xx.h"
#include "utils.h"

#pragma pack(push,1)

typedef struct dac_handle
{
    DAC_TypeDef* inst;

    bool is_initialized_ch1;
    bool is_initialized_ch2;
}typDac_handle;

#pragma pack(pop)

void dac_init(void);
void DAC_setValue_ch1(uint16_t val);
void DAC_setValue_ch2(uint16_t val);