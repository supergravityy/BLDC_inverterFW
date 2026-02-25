#include "../Inc/adc.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

#define ADC_12BIT_HALF_RESOLUTION   (2048UL)
#define ADC_OFFSET_SMPLE_CNT        (10UL)
#define ADC_SAMPLE_15CYC    0x02
#define ADC_SMPR_ALL_15     ( (ADC_SAMPLE_15CYC << 0) | (ADC_SAMPLE_15CYC << 3) | \
                              (ADC_SAMPLE_15CYC << 6) | (ADC_SAMPLE_15CYC << 9) | \
                              (ADC_SAMPLE_15CYC << 12) | (ADC_SAMPLE_15CYC << 15) | \
                              (ADC_SAMPLE_15CYC << 18) | (ADC_SAMPLE_15CYC << 21) )

typAdc_handle vAdc_handler[ADC_PERIPH_NUM];
uint32_t vAdc_currOffsets[ADC_PERIPH_NUM]; // 전류 오프셋 보정값 저장 배열

// todo : adc 초기화 -> polling 모드 대신 injection 모드로 바꾸기 (리소스 소모 엄청남) -> 타이머 트리거 사용

void adc_offsetCalib(void)
{
    uint32_t ias_sum = 0, ibs_sum = 0, ics_sum = 0;

    for(int i = 0; i < ADC_OFFSET_SMPLE_CNT; i++)
    {
        // 각 채널을 순차적으로 Polling 변환하여 10번 합산
        ias_sum += adc_conv_rawIus_polling();
        ibs_sum += adc_conv_rawIvs_polling();
        ics_sum += adc_conv_rawIws_polling();
    }

    // 평균값 계산 및 핸들러 저장 (나중에 adc_get_current_A에서 사용)
    vAdc_handler[0].offset_rawVal = (uint16_t)(ias_sum / ADC_OFFSET_SMPLE_CNT);
    vAdc_handler[1].offset_rawVal = (uint16_t)(ibs_sum / ADC_OFFSET_SMPLE_CNT);
    vAdc_handler[2].offset_rawVal = (uint16_t)(ics_sum / ADC_OFFSET_SMPLE_CNT);

    for(int i = 0; i < ADC_PERIPH_NUM; i++)
    {
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
    for(int i = 0; i < ADC_PERIPH_NUM; i++)
    {
        currOffsets[i] = vAdc_handler[i].offset_rawVal;
    }
}

void adc_init(void)
{
    vAdc_handler[0].moduleInst = ADC1;
    vAdc_handler[1].moduleInst = ADC2;
    vAdc_handler[2].moduleInst = ADC3;

    // ADC1 핀설정
    gpio_set_analog(IAS_ADC_Port, 0);
    gpio_set_analog(MOSFET_NTC_Port, 6);
    // ADC2 핀설정
    gpio_set_analog(IBS_ADC_Port, 1);
    gpio_set_analog(THROTTLE_Port, 7);
    // ADC3 핀설정
    gpio_set_analog(ICS_ADC_Port, 2);
    gpio_set_analog(VDC_ADC_Port, 3);

    // ADC 페리페럴 설정
    // 온도센서
    ADC->CCR &= ~(ADC_CCR_TSVREFE | ADC_CCR_VBATE | ADC_CCR_ADCPRE_Msk | 
        ADC_CCR_DMA_Msk | ADC_CCR_DELAY_Msk | ADC_CCR_MULTI_Msk);
    
    // ADC 모듈 채널별 샘플링 시간 설정 -> cap을 충전하는 시간 (샘플 & 홀드)
    vAdc_handler[0].moduleInst->SMPR2 = ADC_SMPR_ALL_15; // 방어적 프로그래밍?
    vAdc_handler[1].moduleInst->SMPR2 = ADC_SMPR_ALL_15;
    vAdc_handler[2].moduleInst->SMPR2 = ADC_SMPR_ALL_15;

    // ADC 모듈 상태 설정 (초기상태)
    vAdc_handler[0].moduleInst->CR1 = 0;    vAdc_handler[0].moduleInst->CR2 = 0;
    vAdc_handler[0].moduleInst->SQR1 = 0;   vAdc_handler[0].moduleInst->SQR2 = 0;
    vAdc_handler[0].moduleInst->SQR3 = 0;

    vAdc_handler[1].moduleInst->CR1 = 0;    vAdc_handler[1].moduleInst->CR2 = 0;
    vAdc_handler[1].moduleInst->SQR1 = 0;   vAdc_handler[1].moduleInst->SQR2 = 0;
    vAdc_handler[1].moduleInst->SQR3 = 0;

    vAdc_handler[2].moduleInst->CR1 = 0;    vAdc_handler[2].moduleInst->CR2 = 0;
    vAdc_handler[2].moduleInst->SQR1 = 0;   vAdc_handler[2].moduleInst->SQR2 = 0;
    vAdc_handler[2].moduleInst->SQR3 = 0;

    // ADC ON
    ADC1->CR2 |= ADC_CR2_ADON;
    ADC2->CR2 |= ADC_CR2_ADON;
    ADC3->CR2 |= ADC_CR2_ADON;

    // 초기화 완료 플래그 설정
    vAdc_handler[0].is_initialized = ((ADC1->CR2 & ADC_CR2_ADON) == ADC_CR2_ADON);
    vAdc_handler[1].is_initialized = ((ADC2->CR2 & ADC_CR2_ADON) == ADC_CR2_ADON);
    vAdc_handler[2].is_initialized = ((ADC3->CR2 & ADC_CR2_ADON) == ADC_CR2_ADON);
}

// 보조 ADC 변환 함수들 (polling 방식)
// NOTE : 상전류 측정 함수는 ISR에서 실행되기에, 보조변환 함수의 작업이 취소되는 경우가 발생할 수 있음 -> 크리티컬 섹션

uint16_t adc_conv_rawNTC_polling(void)
{
	UTILS_DISABLE_ISR(); // critical section start

    vAdc_handler[0].moduleInst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[0].moduleInst->SQR3 |= UTILS_BIT_SHIFT(0,6); // ADC1_IN6 (NTC)
    vAdc_handler[0].moduleInst->CR2 |= ADC_CR2_SWSTART;

    while(!(vAdc_handler[0].moduleInst->SR & ADC_SR_EOC));
    
    vAdc_handler[0].aux_rawVal = vAdc_handler[0].moduleInst->DR;

    UTILS_ENABLE_ISR(); // critical section end

    return vAdc_handler[0].aux_rawVal;
}

uint16_t adc_conv_rawThrottle_polling(void)
{
	UTILS_DISABLE_ISR(); // critical section start

    vAdc_handler[1].moduleInst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[1].moduleInst->SQR3 |= UTILS_BIT_SHIFT(0,7); // ADC2_IN7 (throttle)
    vAdc_handler[1].moduleInst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[1].moduleInst->SR & ADC_SR_EOC));
    
    vAdc_handler[1].aux_rawVal = vAdc_handler[1].moduleInst->DR;

    UTILS_ENABLE_ISR(); // critical section end

    return vAdc_handler[1].aux_rawVal;
}

uint16_t adc_conv_rawVdc_polling(void)
{
	UTILS_DISABLE_ISR(); // critical section start

    vAdc_handler[2].moduleInst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[2].moduleInst->SQR3 |= UTILS_BIT_SHIFT(0,3); // ADC3_IN3 (vdc)
    vAdc_handler[2].moduleInst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[2].moduleInst->SR & ADC_SR_EOC));
    
    vAdc_handler[2].aux_rawVal = vAdc_handler[2].moduleInst->DR;

    UTILS_ENABLE_ISR(); // critical section end

    return vAdc_handler[2].aux_rawVal;
}

// 전류 ADC 변환 함수들 (polling 방식)

uint16_t adc_conv_rawIus_polling(void)
{
    vAdc_handler[0].moduleInst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[0].moduleInst->SQR3 |= UTILS_BIT_SHIFT(0,0); // ADC1_IN0 (Ias)
    vAdc_handler[0].moduleInst->CR2 |= ADC_CR2_SWSTART;

    while(!(vAdc_handler[0].moduleInst->SR & ADC_SR_EOC));
    
    vAdc_handler[0].curr_rawVal = vAdc_handler[0].moduleInst->DR;
    return vAdc_handler[0].curr_rawVal;
}

uint16_t adc_conv_rawIvs_polling(void)
{
    vAdc_handler[1].moduleInst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[1].moduleInst->SQR3 |= UTILS_BIT_SHIFT(0,1); // ADC2_IN1 (Ibs)
    vAdc_handler[1].moduleInst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[1].moduleInst->SR & ADC_SR_EOC));
    
    vAdc_handler[1].curr_rawVal = vAdc_handler[1].moduleInst->DR;
    return vAdc_handler[1].curr_rawVal;
}

uint16_t adc_conv_rawIws_polling(void)
{
    vAdc_handler[2].moduleInst->SQR3 &= ~ADC_SQR3_SQ1_Msk;
    vAdc_handler[2].moduleInst->SQR3 |= UTILS_BIT_SHIFT(0,2); // ADC3_IN2 (Ics)
    vAdc_handler[2].moduleInst->CR2 |= ADC_CR2_SWSTART;
    
    while(!(vAdc_handler[2].moduleInst->SR & ADC_SR_EOC));
    
    vAdc_handler[2].curr_rawVal = vAdc_handler[2].moduleInst->DR;
    return vAdc_handler[2].curr_rawVal;
}