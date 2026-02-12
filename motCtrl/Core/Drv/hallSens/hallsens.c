#include "hallsens.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "exti.h"
#include "../Inc/Gpio.h"
#include "tim.h"
#include "../mtrCtrl/mtrCtrl.h"

#define HALLSENS_CAL_SUM(hallU, hallV, hallW)               ((hallU << 2) | (hallV << 1) | (hallW))

typHall_Handle vHallSens_handler;

void hallsens_update_hallSeq(void)
{
    vHallSens_handler.hallPin_state[HALLSENS_U_IDX] = gpio_read_pin(vHallSens_handler.port[HALLSENS_U_IDX], vHallSens_handler.pin[HALLSENS_U_IDX]);
    vHallSens_handler.hallPin_state[HALLSENS_V_IDX] = gpio_read_pin(vHallSens_handler.port[HALLSENS_V_IDX], vHallSens_handler.pin[HALLSENS_V_IDX]);
    vHallSens_handler.hallPin_state[HALLSENS_W_IDX] = gpio_read_pin(vHallSens_handler.port[HALLSENS_W_IDX], vHallSens_handler.pin[HALLSENS_W_IDX]);

    vHallSens_handler.curr_hallSum = HALLSENS_CAL_SUM(vHallSens_handler.hallPin_state[HALLSENS_U_IDX],
                                                      vHallSens_handler.hallPin_state[HALLSENS_V_IDX],
                                                      vHallSens_handler.hallPin_state[HALLSENS_W_IDX]);
}

void hallsens_init(void)
{
    vHallSens_handler.port[HALLSENS_U_IDX] = HALL_U_Port;
    vHallSens_handler.pin[HALLSENS_U_IDX] = 0;
    vHallSens_handler.port[HALLSENS_V_IDX] = HALL_V_Port;
    vHallSens_handler.pin[HALLSENS_V_IDX] = 1;
    vHallSens_handler.port[HALLSENS_W_IDX] = HALL_W_Port;
    vHallSens_handler.pin[HALLSENS_W_IDX] = 2;

    vHallSens_handler.hallPin_state[HALLSENS_U_IDX] = 0;
    vHallSens_handler.hallPin_state[HALLSENS_V_IDX] = 0;
    vHallSens_handler.hallPin_state[HALLSENS_W_IDX] = 0;
    vHallSens_handler.dutyVal[HALLSENS_U_IDX] = 0;
    vHallSens_handler.dutyVal[HALLSENS_V_IDX] = 0;
    vHallSens_handler.dutyVal[HALLSENS_W_IDX] = 0;

    vHallSens_handler.curr_hallSum = 0;
    vHallSens_handler.prev_hallSum = 0;
    vHallSens_handler.direction = MTR_DIR_STP;
    vHallSens_handler.lastCaptureTime = 0;
    vHallSens_handler.deltaTime = 0;
    vHallSens_handler.rawRpm = 0.0f;

    hallsens_update_hallSeq();

    vHallSens_handler.is_initialized = true;
}

static void hallsens_set_PWMduty(typMtrPhase phases[])
{
    uint8_t idx;
    uint32_t tempDutyVal = 0;

    // 1. PWM 듀티 계산
    // todo : voltageRef가 뭔지 알아보고 대입하기
    vHallSens_handler.dutyVal[HALLSENS_U_IDX] = HALLSENS_DUTY_MAX /* -  */;
    vHallSens_handler.dutyVal[HALLSENS_V_IDX] = HALLSENS_DUTY_MAX /* -  */;
    vHallSens_handler.dutyVal[HALLSENS_W_IDX] = HALLSENS_DUTY_MAX /* -  */;

    // 2. 정지 신호인지 확인
    if(/* todo : voltageRef가 뭔지 알아보고 대입 ||*/ mtrCtrl_getCrlContinue() == false)
    {
        tim_Pwm1_Mute_channel(TIM_SELECT_ALL_LINE);
        tim_Pwm1_setCmpVal(TIM_SELECT_ALL_LINE,0);
        return ;
    }

    // 3. PWM 위상 세팅
    for(idx = 0; idx < HALLSENS_PH_NUM ; idx++)
    {
        if(phases[idx] == MTR_DIR_FWD)      tempDutyVal = vHallSens_handler.dutyVal[idx];
        else if(phases[idx] == MTR_DIR_REV) tempDutyVal = HALLSENS_DUTY_MAX;
        else                                tempDutyVal = 0;

        tim_Pwm1_setCmpVal((typTim_sigLineNum)idx, tempDutyVal);
        tim_Pwm1_Unmute_channel((typTim_sigLineNum)idx);
    }
}

static void hallsens_setPhase_FWD(void)
{
    typMtrPhase phases[HALLSENS_PH_NUM] = {0};
    
#if (USE_INWHEEL == 1)
    switch(vHallSens_handler.curr_hallSum)
    {
        case 5:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 3:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        case 1:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 6:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 4:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        case 2:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        default: phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
    }
#else
    // 소형 BLDC 매핑 (Original Switch-Case 반영)
    switch(vHallSens_handler.curr_hallSum)
    {
        case 6:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 4:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 5:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        case 1:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 3:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 2:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        default: phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
    }
#endif
    hallsens_set_PWMduty(phases);
}

static void hallsens_setPhase_REV(void)
{
    typMtrPhase phases[HALLSENS_PH_NUM] = {0};

#if (USE_INWHEEL == 1)
    // 인휠모터 역방향 매핑
    switch(vHallSens_handler.curr_hallSum)
    {
        case 5:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 3:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        case 1:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 6:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 4:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        case 2:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        default: phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
    }
#else
    // 소형 BLDC 역방향 매핑
    switch(vHallSens_handler.curr_hallSum)
    {
        case 6:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 4:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_REV; break;
        case 5:  phases[HALLSENS_U_IDX] = MTR_DIR_FWD; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        case 1:  phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_REV; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 3:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_FWD; break;
        case 2:  phases[HALLSENS_U_IDX] = MTR_DIR_REV; phases[HALLSENS_V_IDX] = MTR_DIR_FWD; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
        default: phases[HALLSENS_U_IDX] = MTR_DIR_STP; phases[HALLSENS_V_IDX] = MTR_DIR_STP; phases[HALLSENS_W_IDX] = MTR_DIR_STP; break;
    }
#endif
    hallsens_set_PWMduty(phases);
}

void hallsens_update_swtPattern(void)
{
	#if (MOTOR_DIR == MOTOR_DIR_CW)
		hallsens_setPhase_FWD();
	#else
		hallsens_setPhase_REV();
	#endif
}