#include "tim.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../Inc/Gpio.h"

#define TIM_DET_RUNNING_FLAG(reg)   ((reg & TIM_CR1_CEN) == 1)
#define TIM_DET_OUTPUT_FLAG(reg)    ((reg & TIM_BDTR_MOE) == 1)

/* ------------------------------------- */
// NOTE : 현재 인버터에서 사용하는 타이머는 총 2개이다/*  */.
// TIM1 : 게이트 드라이버 IC로 보내는 3상 PWM 신호 생성용
// TIM2 : 홀센서를 이용한 RPM 계산용 (샘플간 delta_t)
/* ------------------------------------- */

typPwm_handle Pwm1_handler;
typTim_handle Tim2_handler;

static void tim_tim2_init(void)
{
    Tim2_handler.inst = TIM2;

    Tim2_handler.inst->PSC = TIM2_PERIOD_TICK;
    Tim2_handler.inst->ARR = TIM2_PRESCALSER_CNT;
    Tim2_handler.inst->CNT = TIM2_INIT_CNT_VAL;

    Tim2_handler.inst->CR1 = TIM_CR1_CEN;

    Tim2_handler.is_running = TIM_DET_RUNNING_FLAG(Tim2_handler.inst->CR1);
    Tim2_handler.is_initialized = true;
}

static void tim_pwm1_init(void)
{
    // todo : gpioe의 레지스터를 수정할 수 있는 api 만들기

    // U-Phase PWM 설정
    gpio_set_alterFunc(PWM_U_TOP_Port, PWM_U_TOP_Pin, 1, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(PWM_U_BOT_Port, PWM_U_BOT_Pin, 1, GPIO_SPD_VERY_HIGH);

    // V-Phase PWM 설정
    gpio_set_alterFunc(PWM_V_TOP_Port, PWM_V_TOP_Pin, 1, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(PWM_V_BOT_Port, PWM_V_BOT_Pin, 1, GPIO_SPD_VERY_HIGH);

    // W-Phase PWM 설정
    gpio_set_alterFunc(PWM_W_TOP_Port, PWM_W_TOP_Pin, 1, GPIO_SPD_VERY_HIGH);
    gpio_set_alterFunc(PWM_W_BOT_Port, PWM_W_BOT_Pin, 1, GPIO_SPD_VERY_HIGH);

    Pwm1_handler.inst = TIM1;

    Pwm1_handler.inst->PSC = TIM1_PRESCALER_CNT; // 216MHz/(0+1) = 216MHz
    Pwm1_handler.inst->ARR = TIM1_PERIOD_TICK;   // 216MHz/CNT_MAX/2 = 20kHz -> center allign
    
    // TIM1_CH1,2,3 = PWM mode 2
    Pwm1_handler.inst->CCMR1 &= (~TIM_CCMR1_CC1S_Msk); // PWM 출력모드로사용
    Pwm1_handler.inst->CCMR1 |= TIM_CCMR1_OC1PE | TIM_CCMR1_OC1M_0 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1M_2 ;
    Pwm1_handler.inst->CCMR1 &= (~TIM_CCMR1_CC2S_Msk);
    Pwm1_handler.inst->CCMR1 |= TIM_CCMR1_OC2PE | TIM_CCMR1_OC2M_0 | TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2;
    Pwm1_handler.inst->CCMR2 &= (~TIM_CCMR2_CC3S_Msk);
    Pwm1_handler.inst->CCMR2 |= TIM_CCMR2_OC3PE | TIM_CCMR2_OC3M_0 | TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2 ;
    
    Pwm1_handler.inst->CCER = 0x0;
    Pwm1_handler.inst->CCER = TIM_CCER_CC1E | TIM_CCER_CC1NE | TIM_CCER_CC2E | TIM_CCER_CC2NE | TIM_CCER_CC3E | TIM_CCER_CC3NE;

    Pwm1_handler.inst->BDTR = TIM_BDTR_OSSI | TIM_BDTR_OSSR | TIM_BDTR_MOE;
    Pwm1_handler.inst->BDTR |= TIM1_DEADTIME_1US_BIT;

    Pwm1_handler.inst->CR2 = TIM_CR2_MMS_1; // 안정적인 전류센싱을 위해 CNT의 한주기가 끝나면 ADC에 신호를 보냄
    Pwm1_handler.inst->CR1 = TIM_CR1_CEN | TIM_CR1_CMS_1; // 센터 얼리인 모드 확정 + 카운트 시작

    Pwm1_handler.isr_count = 0;
    Pwm1_handler.is_running = TIM_DET_RUNNING_FLAG(Pwm1_handler.inst->CR1);
    Pwm1_handler.is_outputing = TIM_DET_OUTPUT_FLAG(Pwm1_handler.inst->BDTR);
    Pwm1_handler.is_initialized = true;
}

static void tim_pwm1_nvic_setting(void)
{
    Pwm1_handler.inst->DIER = TIM_DIER_UIE; // 오버플로 인터럽트 사용

    while(Pwm1_handler.inst->DIER & TIM_CR1_DIR == 1); // 업카운팅 -> 다운카운팅 까지 대기(싱크맞추기용)

    Pwm1_handler.inst->RCR = 0x01;  // 센터 얼라인이기 때문에, 업카운팅->다운카운팅 해야 업데이트 인터럽트 발생
}

void tim_init(void)
{
    tim_pwm1_init();
    tim_tim2_init();

    tim_pwm1_nvic_setting();
}

void tim_Pwm1_Mute_channel(typTim_sigLineNum chNum)
{
    switch(chNum)
    {
        case TIM_SELECT_ALL_LINE:
            Pwm1_handler.inst->BDTR &= ~(TIM_BDTR_MOE);
            Pwm1_handler.is_outputing = TIM_DET_OUTPUT_FLAG(Pwm1_handler.inst->BDTR);
            tim_Pwm1_Mute_channel(TIM_SELECT_U_LINE);
            tim_Pwm1_Mute_channel(TIM_SELECT_V_LINE);
            tim_Pwm1_Mute_channel(TIM_SELECT_W_LINE);
            break;
        case TIM_SELECT_U_LINE:
            Pwm1_handler.inst->CCER &= ~(TIM_CCER_CC1E | TIM_CCER_CC1NE);
            break;
        case TIM_SELECT_V_LINE:
            Pwm1_handler.inst->CCER &= ~(TIM_CCER_CC2E | TIM_CCER_CC2NE);
            break;
        case TIM_SELECT_W_LINE:
            Pwm1_handler.inst->CCER &= ~(TIM_CCER_CC3E | TIM_CCER_CC3NE);
            break;
        default:
            break;
    }
}

void tim_Pwm1_Unmute_channel(typTim_sigLineNum chNum)
{   
    switch(chNum)
    {
        case TIM_SELECT_ALL_LINE:
            tim_Pwm1_Unmute_channel(TIM_SELECT_U_LINE);
            tim_Pwm1_Unmute_channel(TIM_SELECT_V_LINE);
            tim_Pwm1_Unmute_channel(TIM_SELECT_W_LINE);
            Pwm1_handler.inst->BDTR |= TIM_BDTR_MOE;
            Pwm1_handler.is_outputing = TIM_DET_OUTPUT_FLAG(Pwm1_handler.inst->BDTR);
            break;
        case TIM_SELECT_U_LINE:
            Pwm1_handler.inst->CCER |= (TIM_CCER_CC1E | TIM_CCER_CC1NE);
            break;
        case TIM_SELECT_V_LINE:
            Pwm1_handler.inst->CCER |= (TIM_CCER_CC2E | TIM_CCER_CC2NE);
            break;
        case TIM_SELECT_W_LINE:
            Pwm1_handler.inst->CCER |= (TIM_CCER_CC3E | TIM_CCER_CC3NE);
            break;
        default:
            break;
    }
}

void tim_Pwm1_setCompareVal(typTim_sigLineNum chNum, uint32_t ch_duty)
{
    switch(chNum)
    {
        case TIM_SELECT_ALL_LINE:
            Pwm1_handler.inst->CCR1 = ch_duty;
            break;
        case TIM_SELECT_U_LINE:
            Pwm1_handler.inst->CCR1 = ch_duty;
            //Pwm1_handler.u_posDuty = ch_duty; // todo : uint32_t -> float 필요
            break;
        case TIM_SELECT_V_LINE:
            Pwm1_handler.inst->CCR2 = ch_duty;
            //Pwm1_handler.v_posDuty = ch_duty;
            break;
        case TIM_SELECT_W_LINE:
            Pwm1_handler.inst->CCR3 = ch_duty;
            //Pwm1_handler.w_posDuty = ch_duty;
            break;
        default:
            break;
    }
}

uint32_t tim_getTim2_cnt(void)
{
    return Tim2_handler.inst->CNT;
}

uint32_t tim_getTim2_maxCnt(void)
{
    return Tim2_handler.inst->ARR;
}

uint32_t tim_getPwm1_cnt(void)
{
    return Pwm1_handler.inst->CNT;
}

uint32_t tim_getPwm1_maxCnt(void)
{
    return Pwm1_handler.inst->ARR;
}

bool tim_getPwm1_ISR_flg(void)
{
    return (Pwm1_handler.inst->SR & TIM_SR_UIF);
}

void tim_clrPwm1_ISR_flg(void)
{
    Pwm1_handler.inst->SR &= ~TIM_SR_UIF;
}