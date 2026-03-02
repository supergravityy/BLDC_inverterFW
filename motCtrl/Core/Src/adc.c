#include "../Inc/adc.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

#define ADC_HANDLER_U_CH_IDX        (0)
#define ADC_HANDLER_V_CH_IDX        (1)
#define ADC_HANDLER_W_CH_IDX        (2)

#define ADC_12BIT_HALF_RESOLUTION   (2048UL)
#define ADC_OFFSET_SMPLE_CNT        (10UL)
#define ADC_SAMPLE_15CYC    0x02
#define ADC_SMPR_ALL_15     ( (ADC_SAMPLE_15CYC << 0) | (ADC_SAMPLE_15CYC << 3) | \
                              (ADC_SAMPLE_15CYC << 6) | (ADC_SAMPLE_15CYC << 9) | \
                              (ADC_SAMPLE_15CYC << 12) | (ADC_SAMPLE_15CYC << 15) | \
                              (ADC_SAMPLE_15CYC << 18) | (ADC_SAMPLE_15CYC << 21) )

typAdc_handle vAdc_handler[ADC_MODULE_NUM];

void adc_offsetCalib(void)
{
    uint32_t ius_sum = 0, ivs_sum = 0, iws_sum = 0;

    for(int i = 0; i < ADC_OFFSET_SMPLE_CNT; i++)
    {
        // 각 채널을 순차적으로 Polling 변환하여 10번 합산
        ius_sum += adc_conv_rawIus_polling();
        ivs_sum += adc_conv_rawIvs_polling();
        iws_sum += adc_conv_rawIws_polling();
    }

    // 평균값 계산 및 핸들러 저장 (나중에 adc_get_current_A에서 사용)
    vAdc_handler[ADC_HANDLER_U_CH_IDX].offset_rawVal = (uint16_t)(ius_sum / ADC_OFFSET_SMPLE_CNT);
    vAdc_handler[ADC_HANDLER_V_CH_IDX].offset_rawVal = (uint16_t)(ivs_sum / ADC_OFFSET_SMPLE_CNT);
    vAdc_handler[ADC_HANDLER_W_CH_IDX].offset_rawVal = (uint16_t)(iws_sum / ADC_OFFSET_SMPLE_CNT);

    for(int i = 0; i < ADC_MODULE_NUM; i++)
    {
        // 회로 구조상 영점(1.65V)보다 아래로 오차가 생기는 건 비정상적이지만 무시할 수준이라 보고, 그냥 정상 영점인 2048로 취급
        if(vAdc_handler[i].offset_rawVal <= ADC_12BIT_HALF_RESOLUTION)
        {
            vAdc_handler[i].offset_rawVal = ADC_12BIT_HALF_RESOLUTION;
        }
        else
        {
            vAdc_handler[i].offset_rawVal = vAdc_handler[i].offset_rawVal - ADC_12BIT_HALF_RESOLUTION;
        }
    }
}

void adc_getOffsetCalib_val(uint32_t currOffsets[])
{
    for(int i = 0; i < ADC_MODULE_NUM; i++)
    {
        currOffsets[i] = vAdc_handler[i].offset_rawVal;
    }
}

void adc_init(void)
{
    // 1. 핀 설정
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst = ADC1;
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst = ADC2;
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst = ADC3;

    gpio_set_analog(IAS_ADC_Port, 0);
    gpio_set_analog(MOSFET_NTC_Port, 6);
    gpio_set_analog(IBS_ADC_Port, 1);
    gpio_set_analog(THROTTLE_Port, 7);
    gpio_set_analog(ICS_ADC_Port, 2);
    gpio_set_analog(VDC_ADC_Port, 3);

    // ADC 페리페럴 설정
    // 온도센서
    ADC->CCR &= ~(ADC_CCR_TSVREFE | ADC_CCR_VBATE | ADC_CCR_ADCPRE_Msk | 
        ADC_CCR_DMA_Msk | ADC_CCR_DELAY_Msk | ADC_CCR_MULTI_Msk);
    
    // ADC 모듈 채널별 샘플링 시간 설정 -> cap을 충전하는 시간 (샘플 & 홀드)
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SMPR2 = ADC_SMPR_ALL_15; // 방어적 프로그래밍?
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SMPR2 = ADC_SMPR_ALL_15;
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SMPR2 = ADC_SMPR_ALL_15;

    // ADC 모듈 상태 설정 (초기상태)
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->CR1 = 0;    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->CR2 = 0;
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR1 = 0;   vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR2 = 0;
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR3 = 0;

    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->CR1 = 0;    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->CR2 = 0;
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR1 = 0;   vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR2 = 0;
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR3 = 0;

    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->CR1 = 0;    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->CR2 = 0;
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR1 = 0;   vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR2 = 0;
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR3 = 0;

    // ADC ON
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC2->CR2 |= ADC_CR2_ADON;
    ADC3->CR2 |= ADC_CR2_ADON;

    // 초기화 완료 플래그 설정
    vAdc_handler[ADC_HANDLER_U_CH_IDX].is_initialized = ((ADC1->CR2 & ADC_CR2_ADON) == ADC_CR2_ADON);
    vAdc_handler[ADC_HANDLER_V_CH_IDX].is_initialized = ((ADC2->CR2 & ADC_CR2_ADON) == ADC_CR2_ADON);
    vAdc_handler[ADC_HANDLER_W_CH_IDX].is_initialized = ((ADC3->CR2 & ADC_CR2_ADON) == ADC_CR2_ADON);
}

// 보조 ADC 변환 함수들 (polling 방식)
// NOTE : 상전류 측정 함수는 ISR에서 실행되기에, 보조변환 함수의 작업이 취소되는 경우가 발생할 수 있음 -> 크리티컬 섹션

uint16_t adc_conv_rawNTC_polling(void)
{
	UTILS_DISABLE_ISR(); // critical section start

    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR3 |= UTILS_BIT_SHIFT(0,6); // ADC1_IN6 (NTC)
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->CR2 |= ADC_CR2_SWSTART;

    while(!(vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SR & ADC_SR_EOC));
    
    vAdc_handler[ADC_HANDLER_U_CH_IDX].aux_rawVal = vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->DR;

    UTILS_ENABLE_ISR(); // critical section end

    return vAdc_handler[ADC_HANDLER_U_CH_IDX].aux_rawVal;
}

uint16_t adc_conv_rawThrottle_polling(void)
{
	UTILS_DISABLE_ISR(); // critical section start

    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR3 |= UTILS_BIT_SHIFT(0,7); // ADC2_IN7 (throttle)
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SR & ADC_SR_EOC));
    
    vAdc_handler[ADC_HANDLER_V_CH_IDX].aux_rawVal = vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->DR;

    UTILS_ENABLE_ISR(); // critical section end

    return vAdc_handler[ADC_HANDLER_V_CH_IDX].aux_rawVal;
}

uint16_t adc_conv_rawVdc_polling(void)
{
	UTILS_DISABLE_ISR(); // critical section start

    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR3 |= UTILS_BIT_SHIFT(0,3); // ADC3_IN3 (vdc)
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SR & ADC_SR_EOC));
    
    vAdc_handler[ADC_HANDLER_W_CH_IDX].aux_rawVal = vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->DR;

    UTILS_ENABLE_ISR(); // critical section end

    return vAdc_handler[ADC_HANDLER_W_CH_IDX].aux_rawVal;
}

// 전류 ADC 변환 함수들 (polling 방식)

uint16_t adc_conv_rawIus_polling(void)
{
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SQR3 |= UTILS_BIT_SHIFT(0,0); // ADC1_IN0 (Ias)
    vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->CR2 |= ADC_CR2_SWSTART;

    while(!(vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->SR & ADC_SR_EOC));
    
    vAdc_handler[ADC_HANDLER_U_CH_IDX].curr_rawVal = vAdc_handler[ADC_HANDLER_U_CH_IDX].inst->DR;
    return vAdc_handler[ADC_HANDLER_U_CH_IDX].curr_rawVal;
}

uint16_t adc_conv_rawIvs_polling(void)
{
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SQR3 |= UTILS_BIT_SHIFT(0,1); // ADC2_IN1 (Ibs)
    vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->SR & ADC_SR_EOC));
    
    vAdc_handler[ADC_HANDLER_V_CH_IDX].curr_rawVal = vAdc_handler[ADC_HANDLER_V_CH_IDX].inst->DR;
    return vAdc_handler[ADC_HANDLER_V_CH_IDX].curr_rawVal;
}

uint16_t adc_conv_rawIws_polling(void)
{
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SQR3 |= UTILS_BIT_SHIFT(0,2); // ADC3_IN2 (Ics)
    vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->SR & ADC_SR_EOC));
    
    vAdc_handler[ADC_HANDLER_W_CH_IDX].curr_rawVal = vAdc_handler[ADC_HANDLER_W_CH_IDX].inst->DR;
    return vAdc_handler[ADC_HANDLER_W_CH_IDX].curr_rawVal;
}