#pragma once 

#include "stm32f767xx.h"
#include "utils.h"

#define PWM1_PERIOD_TICK        (5400UL)
#define PWM1_PRESCALER_CNT      (0)
#define PWM1_DEADTIME_1US_BIT   (0xB4)
#define PWM1_INIT_CNT_VAL       (0)

#define TIM2_PERIOD_TICK        (54000000UL - 1UL)
#define TIM2_PRESCALSER_CNT     (4UL - 1UL)
#define TIM2_INIT_CNT_VAL       (0)


typedef enum tim_sigLineNum
{
    TIM_SELECT_ALL_LINE = 0,
    TIM_SELECT_U_LINE,
    TIM_SELECT_V_LINE,
    TIM_SELECT_W_LINE
}typTim_sigLineNum;

typedef enum tim_phaseSt
{
    TIM_PHASE_OFF = 0,
    TIM_PHASE_CONTROL,
    TIM_PHASE_GND
}typTim_phaseSt;

typedef struct tim_handle
{
    TIM_TypeDef* inst;

    bool is_running;
    bool is_initialized;
}typTim_handle;

typedef struct pwm_handle
{
    TIM_TypeDef* inst;

    bool is_running;
    bool is_outputing;
    bool is_initialized;

    float period_us;
    float u_posDuty;
    float v_posDuty;
    float w_posDuty;
}typPwm_handle;


void tim_init(void);
void tim_Pwm1_Mute_channel(typTim_sigLineNum chNum);
void tim_Pwm1_Unmute_channel(typTim_sigLineNum chNum);
void tim_Pwm1_setCmpVal(typTim_sigLineNum chNum, uint32_t ch_duty);

uint32_t tim_getTim2_cnt(void);
uint32_t tim_getTim2_maxCnt(void);

uint32_t tim_getPwm1_cnt(void);
uint32_t tim_getPwm1_maxCnt(void);

bool tim_getPwm1_ISR_flg(void);
void tim_clrPwm1_ISR_flg(void);
void tim_pwm1_nvic_counterSet(void);
