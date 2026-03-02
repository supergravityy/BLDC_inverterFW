#pragma once

#include "stm32f767xx.h"
#include "utils.h"

#define ADC_MODULE_NUM      (3UL)

typedef struct adc_handle
{
    ADC_TypeDef* inst;

    // 데이터 저장 (Raw 값)
    uint16_t curr_rawVal;   // 전류 측정값 -> main
    uint16_t aux_rawVal;    // 보조 측정값 -> throttle, temperature, DCvolt 

    // 실제 물리 값으로 변환된 데이터 (float)
    uint16_t offset_rawVal;         // 오프셋 보정용 Raw 값 저장

    bool is_initialized;
}typAdc_handle;

void adc_init(void);
uint16_t adc_conv_rawNTC_polling(void);
uint16_t adc_conv_rawThrottle_polling(void);
uint16_t adc_conv_rawVdc_polling(void);
uint16_t adc_conv_rawIus_polling(void);
uint16_t adc_conv_rawIvs_polling(void);
uint16_t adc_conv_rawIws_polling(void);
void adc_offsetCalib(void);
void adc_getOffsetCalib_val(uint32_t* currOffsets);