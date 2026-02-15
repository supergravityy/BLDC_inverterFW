#include "hallsens.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "exti.h"
#include "../Inc/Gpio.h"
#include "tim.h"
#include "../mtrCtrl/mtrCtrl.h"

#define HALLSENS_CNT_MAXTICK                (TIM2_PERIOD_TICK)
#define HALLSENS_CLK_FREQ                   (54000000.0f)
#define HALLSENS_MIN_DELTA_TICK             (500UL)
#define HALLSENS_EDGES_PER_REV              (HALL_EDGES_PER_REV)

// delta_time이 0이 아닐 때만 RPM 계산
// 가정: 한 회전당 6개의 홀 센서 이벤트 발생 (6 edges per revolution)
// Timer 주파수: 54MHz
// RPM 계산 공식: RPM = (60 * Clock_Frequency) / (Edges_per_Revolution * delta_time)
#define HALLSENS_CAL_MOTOR_RPM(dt)                  ((60.f * HALLSENS_CLK_FREQ) / ((float)dt * HALLSENS_EDGES_PER_REV))
#define HALLSENS_CAL_SUM(hallU, hallV, hallW)       ((hallU << 2) | (hallV << 1) | (hallW))

// center 얼라인드 모드이기에 CCR 값이 높아질 수록 듀티가 낮아지는 형태
#define HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(duty)    (HALLSENS_DUTY_MAX - (duty))

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

    vHallSens_handler.prevTick = 0;
    vHallSens_handler.currTick = 0;
    vHallSens_handler.deltaTick = 0.0f;
    vHallSens_handler.rawMtrRPM = 0;
    vHallSens_handler.filteredMtrRPM = 0;

    hallsens_update_hallSeq();

    vHallSens_handler.is_initialized = true;
}

static void hallsens_set_PWMduty(typMtrPhase phases[])
{
    uint8_t idx;
    uint32_t tempCCR_refVal;

    // 1. PWM 듀티 계산
    tempCCR_refVal = mtrCtrl_getFinalCCR_refVal();
    vHallSens_handler.dutyVal[HALLSENS_U_IDX] = HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(tempCCR_refVal);
    vHallSens_handler.dutyVal[HALLSENS_V_IDX] = HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(tempCCR_refVal);
    vHallSens_handler.dutyVal[HALLSENS_W_IDX] = HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(tempCCR_refVal);

    // 2. 정지 신호인지 확인
    if(tempCCR_refVal == 0 || mtrCtrl_getCtrlContinue() == false)
    {
        tim_Pwm1_Mute_channel(TIM_SELECT_ALL_LINE);
        tim_Pwm1_setCmpVal(TIM_SELECT_ALL_LINE,0);
        return ;
    }

    // 3. PWM 위상 세팅
    for(idx = 0; idx < HALLSENS_PH_NUM ; idx++)
    {
        if(phases[idx] == MTR_DIR_FWD)      tempCCR_refVal = vHallSens_handler.dutyVal[idx];
        else if(phases[idx] == MTR_DIR_REV) tempCCR_refVal = HALLSENS_DUTY_MAX;
        else                                tempCCR_refVal = 0;

        tim_Pwm1_setCmpVal((typTim_sigLineNum)idx, tempCCR_refVal);
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

#if (MOTOR_DIR == MOTOR_DIR_CCW)
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
#endif

void hallsens_update_swtPattern(void)
{
	#if (MOTOR_DIR == MOTOR_DIR_CW)
		hallsens_setPhase_FWD();
	#else
		hallsens_setPhase_REV();
	#endif
}

/*-------------------------------------*/
// 모터 rpm 계산
/*-------------------------------------*/

void hallsens_cal_motorRPM(void)
{
    vHallSens_handler.currTick = tim_getTim2_cnt();

    if (vHallSens_handler.currTick >= HALLSENS_CNT_MAXTICK)
    {
        vHallSens_handler.deltaTick = vHallSens_handler.currTick - vHallSens_handler.prevTick;
    }
    else // 오버플로우 발생: 타이머가 ARR 값에 도달하여 리셋됨
    {
        vHallSens_handler.deltaTick = (HALLSENS_CNT_MAXTICK - vHallSens_handler.prevTick) + (vHallSens_handler.currTick + 1);
    }

    vHallSens_handler.prevTick = vHallSens_handler.currTick;

    if (vHallSens_handler.deltaTick > HALLSENS_MIN_DELTA_TICK) // noise filtering
    {
        vHallSens_handler.rawMtrRPM = HALLSENS_CAL_MOTOR_RPM(vHallSens_handler.deltaTick);

        // 저역통과필터 적용
        vHallSens_handler.filteredMtrRPM = utils_LPF_RPM_filter(vHallSens_handler.rawMtrRPM);
    }
}

float hallsens_get_motorRPM(void)
{
    return vHallSens_handler.filteredMtrRPM;
}
