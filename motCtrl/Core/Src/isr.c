#include "isr.h"
#include "utils.h"
#include "tim.h"
#include "../Drv/tasksch/tasksch.h"
#include "../Drv/hallSens/hallsens.h"
#include "../Drv/sensing/sensing.h"
#include "../Drv/mtrCtrl/mtrCtrl.h"

uint32_t isr_pwm1_intrrpt_cnt_debug;

void SysTick_Handler(void)
{
    tasksch_timeManager();
}

// 원래 EXTI0~4 까지는 단 하나의 핀하고 연결(SYSCFG->EXTICR)되서 PR레지스터만 clr해주면 됨
// 하지만, 전기적인 노이즈 때문에 PR을 한번 더 확인함 방어적 프로그래밍임
void EXTI0_IRQHandler(void)
{
    if (exti_check_extiPR(0))
    {
        exti_clear_extiPR(0); // 인터럽트 플래그 클리어
        hallsens_update_hallSeq();
        hallsens_cal_motorRPM();
    }
}

void EXTI1_IRQHandler(void)
{
    if (exti_check_extiPR(1))
    {
        exti_clear_extiPR(1); // 인터럽트 플래그 클리어
        hallsens_update_hallSeq();
        hallsens_cal_motorRPM();
    }
}

void EXTI2_IRQHandler(void)
{
    if (exti_check_extiPR(2))
    {
        exti_clear_extiPR(2); // 인터럽트 플래그 클리어
        hallsens_update_hallSeq();
        hallsens_cal_motorRPM();
    }
}

void TIM1_UP_TIM10_IRQHandler(void) // 20KHz 로 호출됨
{
	isr_pwm1_intrrpt_cnt_debug++;
	hallsens_check_zeroSpd();
	mtrCtrl_chkErrSt(MTRCTRL_ERR_OVER_CURRENT);

    if (tim_getPwm1_ISR_flg() == true)
    {
        tim_clrPwm1_ISR_flg();

        if (mtrCtrl_getPeriphInit() == true && mtrCtrl_getAppInit_flg() == true)
        {
            // PI제어일때도 역시 스로틀이 눌려야 실질적으로 모터가 동작함
            if(mtrCtrl_getErrCode() == MTRCTRL_ERR_NONE
            		&& mtrCtrl_getCtrlContinue() == true
					&& throttle_get_validateFlg() == true)
            {
                tim_Pwm1_Unmute_channel(TIM_SELECT_OUTPUT_FLG);

                hallsens_update_hallSeq();
                hallsens_update_swtPattern();
            }
            else
            {
                tim_Pwm1_Mute_channel(TIM_SELECT_OUTPUT_FLG);
            }
        }
    }
    hallsens_filtering_rawRPM();
}
