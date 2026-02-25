#include "tim.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"
#include "../Inc/adc.h"

#define TIM_DET_RUNNING_FLAG(reg)   ((reg & TIM_CR1_CEN) == 1)
#define TIM_DET_OUTPUT_FLAG(reg)    ((reg & TIM_BDTR_MOE) == 1)

/* ------------------------------------- */
// NOTE : 현재 인버터에서 사용하는 타이머는 총 2개이다/*  */.
// TIM1 : 게이트 드라이버 IC로 보내는 3상 PWM 신호 생성용
// TIM2 : 홀센서를 이용한 RPM 계산용 (샘플간 delta_t)
/* ------------------------------------- */

typPwm_handle vPwm1_handler;
typTim_handle vTim2_handler;

static void tim_tim2_init(void)
{
    vTim2_handler.inst = TIM2;

    vTim2_handler.inst->PSC = TIM2_PRESCALSER_CNT;
    vTim2_handler.inst->ARR = TIM2_PERIOD_TICK;
    vTim2_handler.inst->CNT = TIM2_INIT_CNT_VAL;

    vTim2_handler.inst->CR1 = TIM_CR1_CEN;

    vTim2_handler.is_running = TIM_DET_RUNNING_FLAG(vTim2_handler.inst->CR1);
    vTim2_handler.is_initialized = true;
}

static float tim_pwm1_cal_period_us(void)
{
    float f_clk_mhz = UTILS_F_CLK_MZ;
    float psc = (float)(vPwm1_handler.inst->PSC + 1);
    float arr = (float)(vPwm1_handler.inst->ARR);
    bool ctrAlgn = ((vPwm1_handler.inst->CR1 & TIM_CR1_CMS_Msk) != 0);

    if(ctrAlgn)
        return (psc * arr * 2.0f) / f_clk_mhz;
    else
        return (psc * arr) / f_clk_mhz;
}

static void tim_pwm1_init(void)
{
    // 1. PWM 채널 핀 설정
    gpio_set_alterFunc(PWM_U_TOP_Port, 9, 1, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(PWM_U_BOT_Port, 8, 1, GPIO_SPD_VERY_HIGH);

    gpio_set_alterFunc(PWM_V_TOP_Port, 11, 1, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(PWM_V_BOT_Port, 10, 1, GPIO_SPD_VERY_HIGH);

    gpio_set_alterFunc(PWM_W_TOP_Port, 13, 1, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(PWM_W_BOT_Port, 12, 1, GPIO_SPD_VERY_HIGH);

    // 2. 타이머 주기 설정
    vPwm1_handler.inst = TIM1;
    vPwm1_handler.inst->PSC = PWM1_PRESCALER_CNT; // 216MHz/(0+1) = 216MHz
    vPwm1_handler.inst->ARR = PWM1_PERIOD_TICK;   // 216MHz/CNT_MAX/2 = 20kHz -> center allign
    
    // 3. PWM 모드2 설정 및 프리로드 기능 활성화
    vPwm1_handler.inst->CCMR1 &= (~TIM_CCMR1_CC1S_Msk); // PWM 출력모드로사용
    vPwm1_handler.inst->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 ;
    vPwm1_handler.inst->CCMR1 &= (~TIM_CCMR1_CC2S_Msk);
    vPwm1_handler.inst->CCMR1 |= TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
    vPwm1_handler.inst->CCMR2 &= (~TIM_CCMR2_CC3S_Msk);
    vPwm1_handler.inst->CCMR2 |= TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 ;

    // 4. 데드타임 설정
    vPwm1_handler.inst->BDTR = TIM_BDTR_OSSI | TIM_BDTR_OSSR | TIM_BDTR_MOE;
    vPwm1_handler.inst->BDTR |= PWM1_DEADTIME_1US_BIT;

    // 5. 카운터 세팅 및 카운트 시작
    vPwm1_handler.inst->CR2 = TIM_CR2_MMS_1; // 안정적인 전류센싱을 위해 CNT의 한주기가 끝나면 ADC에 신호를 보냄
    vPwm1_handler.inst->CR1 = TIM_CR1_CEN | TIM_CR1_CMS_0 | TIM_CR1_CMS_1 | TIM_CR1_URS;
    // 센터 얼리인 모드 확정(카운터가 증가->감소가 완벽히 이루어져야 intrpt 실행) + 카운트 시작 + CCR 업데이트 시점 (글리칭현상) = update 인터럽트 발생시점

    // 6. 나머지 객체 멤버 설정
    vPwm1_handler.period_us = tim_pwm1_cal_period_us();
    vPwm1_handler.is_running = TIM_DET_RUNNING_FLAG(vPwm1_handler.inst->CR1);
    vPwm1_handler.is_outputing = TIM_DET_OUTPUT_FLAG(vPwm1_handler.inst->BDTR);
    vPwm1_handler.is_initialized = true;
}

void tim_pwm1_nvic_counterSet(void)
{
    vPwm1_handler.inst->DIER = TIM_DIER_UIE; // 오버플로 인터럽트 사용

    NVIC_EnableIRQ(TIM1_UP_TIM10_IRQn);

    while((vPwm1_handler.inst->CR1 & TIM_CR1_DIR) == 0); // 업카운팅 -> 다운카운팅 까지 대기(싱크맞추기용)

    vPwm1_handler.inst->RCR = 0x01;  // 센터 얼라인이기 때문에, 업카운팅->다운카운팅 해야 업데이트 인터럽트 발생
}

void tim_init(void)
{
    tim_pwm1_init();
    tim_tim2_init();

    tim_Pwm1_Mute_channel(TIM_SELECT_OUTPUT_FLG);
    tim_Pwm1_Mute_channel(TIM_SELECT_U_LINE);
    tim_Pwm1_Mute_channel(TIM_SELECT_V_LINE);
    tim_Pwm1_Mute_channel(TIM_SELECT_W_LINE);
}

void tim_Pwm1_Mute_channel(typTim_sigLineNum chNum)
{
    switch(chNum)
    {
        case TIM_SELECT_OUTPUT_FLG:
            vPwm1_handler.inst->BDTR &= ~(TIM_BDTR_MOE);
            vPwm1_handler.is_outputing = TIM_DET_OUTPUT_FLAG(vPwm1_handler.inst->BDTR);
            vPwm1_handler.inst->CCR1 = 0;
            vPwm1_handler.inst->CCR2 = 0;
            vPwm1_handler.inst->CCR3 = 0;
            break;
        case TIM_SELECT_U_LINE:
        	vPwm1_handler.inst->CCR1 = 0;
            vPwm1_handler.inst->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE);
            break;
        case TIM_SELECT_V_LINE:
        	vPwm1_handler.inst->CCR2 = 0;
            vPwm1_handler.inst->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2NE);
            break;
        case TIM_SELECT_W_LINE:
        	vPwm1_handler.inst->CCR3 = 0;
            vPwm1_handler.inst->CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3NE);
            break;
        default:
            break;
    }
}

inline void tim_Pwm1_Unmute_channel(typTim_sigLineNum chNum)
{   
    switch(chNum)
    {
        case TIM_SELECT_OUTPUT_FLG:

            vPwm1_handler.inst->BDTR |= TIM_BDTR_MOE;
            vPwm1_handler.is_outputing = TIM_DET_OUTPUT_FLAG(vPwm1_handler.inst->BDTR);
            break;
        case TIM_SELECT_U_LINE:
            vPwm1_handler.inst->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC1NE);
            break;
        case TIM_SELECT_V_LINE:
            vPwm1_handler.inst->CCER |= (TIM_CCER_CC2E | TIM_CCER_CC2NE);
            break;
        case TIM_SELECT_W_LINE:
            vPwm1_handler.inst->CCER |= (TIM_CCER_CC3E | TIM_CCER_CC3NE);
            break;
        default:
            break;
    }
}

inline void tim_Pwm1_setCmpVal(typTim_sigLineNum chNum, uint32_t ch_duty)
{
    float max_tick = (float)vPwm1_handler.inst->ARR;
    if (max_tick == 0) return;

    float duty_ratio = (float)ch_duty / max_tick;

    switch(chNum)
    {
        case TIM_SELECT_OUTPUT_FLG:
            vPwm1_handler.inst->CCR1 = ch_duty;
            vPwm1_handler.inst->CCR2 = ch_duty;
            vPwm1_handler.inst->CCR3 = ch_duty;
            vPwm1_handler.u_posDuty = duty_ratio;
            vPwm1_handler.v_posDuty = duty_ratio;
            vPwm1_handler.w_posDuty = duty_ratio;
            break;
        case TIM_SELECT_U_LINE:
            vPwm1_handler.inst->CCR1 = ch_duty;
            vPwm1_handler.u_posDuty = duty_ratio; 
            break;
        case TIM_SELECT_V_LINE:
            vPwm1_handler.inst->CCR2 = ch_duty;
            vPwm1_handler.v_posDuty = duty_ratio;
            break;
        case TIM_SELECT_W_LINE:
            vPwm1_handler.inst->CCR3 = ch_duty;
            vPwm1_handler.w_posDuty = duty_ratio;
            break;
        default:
            break;
    }
}

uint32_t tim_getTim2_cnt(void)
{
    return vTim2_handler.inst->CNT;
}

uint32_t tim_getTim2_maxCnt(void)
{
    return vTim2_handler.inst->ARR;
}

uint32_t tim_getPwm1_cnt(void)
{
    return vPwm1_handler.inst->CNT;
}

uint32_t tim_getPwm1_maxCnt(void)
{
    return vPwm1_handler.inst->ARR;
}

bool tim_getPwm1_ISR_flg(void)
{
    return (vPwm1_handler.inst->SR & TIM_SR_UIF);
}

void tim_clrPwm1_ISR_flg(void)
{
    vPwm1_handler.inst->SR &= ~TIM_SR_UIF;
}
