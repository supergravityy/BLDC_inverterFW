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
#define HALLSENS_EDGES_PER_REV              (UTILS_HALL_EDGES_PER_REV)

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

    vHallSens_handler.prevTick = 0;
    vHallSens_handler.currTick = 0;
    vHallSens_handler.deltaTick = 0.0f;
    vHallSens_handler.rawMtrRPM = 0;
    vHallSens_handler.new_MtrRPM = 0;
    vHallSens_handler.old_MtrRPM = 0;
    vHallSens_handler.zeroSpd_cnt = 0;

    hallsens_update_hallSeq();

    vHallSens_handler.is_initialized = true;
}

inline static void hallsens_set_PWMduty(typMtrPhase phases[])
{
    uint8_t idx;
    uint32_t tempCCR_refVal = mtrCtrl_getFinalCCR_refVal();
    typTim_sigLineNum tempLineNum;

    // 1. 정지 신호인지 확인

    if (tempCCR_refVal == 0 || mtrCtrl_getCtrlContinue() == false)
    {
		tim_Pwm1_Mute_channel(TIM_SELECT_OUTPUT_FLG);
		tim_Pwm1_setCmpVal(TIM_SELECT_OUTPUT_FLG, 0);
		return;
	}

    // 2. 센터얼라인드 모드의 듀티값과 파형의 전력값이 반비례임을 고려
    vHallSens_handler.dutyVal[HALLSENS_U_IDX] = HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(tempCCR_refVal);
    vHallSens_handler.dutyVal[HALLSENS_V_IDX] = HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(tempCCR_refVal);
    vHallSens_handler.dutyVal[HALLSENS_W_IDX] = HALLSENS_CAL_DUTY_CNTR_ALIGNED_PWM(tempCCR_refVal);

    // 3. 각 상의 상태에 따른 제어
	for (idx = 0; idx < HALLSENS_PH_NUM; idx++)
	{
        tempLineNum = idx + 1;

		if (phases[idx] == MTR_DIR_FWD) // (1) 전압 인가 상
		{
			// 상측 MOSFET의 게이트에 PWM 파형을 출력
			tim_Pwm1_setCmpVal(tempLineNum, vHallSens_handler.dutyVal[idx]);
			tim_Pwm1_Unmute_channel(tempLineNum);
		}
		else if (phases[idx] == MTR_DIR_REV) // (-1) 전류가 빠져나가는 상 (GND)
		{
			// 하측 MOSFET만 켜지게 함
			tim_Pwm1_setCmpVal(tempLineNum, HALLSENS_DUTY_MAX);
			tim_Pwm1_Unmute_channel(tempLineNum);
		}
		else // (0) 플로팅 상 (STP)
		{
			// CCR 값과 상관없이 출력을 아예 차단 (floating)
			tim_Pwm1_Mute_channel(tempLineNum);
		}
	}
}

#if (UTILS_MOTOR_DIR == UTILS_MOTOR_DIR_CW)
inline static void hallsens_setPhase_FWD(void)
{
    typMtrPhase phases[HALLSENS_PH_NUM] = {0};
    

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
    hallsens_set_PWMduty(phases);
}
#endif

#if (UTILS_MOTOR_DIR == UTILS_MOTOR_DIR_CCW)
inline static void hallsens_setPhase_REV(void)
{
    typMtrPhase phases[HALLSENS_PH_NUM] = {0};

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
    hallsens_set_PWMduty(phases);
}
#endif

void hallsens_update_swtPattern(void)
{
	#if (UTILS_MOTOR_DIR == UTILS_MOTOR_DIR_CW)
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

    if (vHallSens_handler.currTick >= vHallSens_handler.prevTick)
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
    }
}

void hallsens_filtering_rawRPM(void)
{
	// 저역통과필터 적용
	vHallSens_handler.new_MtrRPM = utils_LPF_RPM_filter(vHallSens_handler.rawMtrRPM);
}

void hallsens_check_zeroSpd(void)
{
	if(vHallSens_handler.new_MtrRPM == vHallSens_handler.old_MtrRPM)
	{
		vHallSens_handler.zeroSpd_cnt++;

		if(vHallSens_handler.zeroSpd_cnt >= HALLSENS_ZEROSPD_CNT)
		{
			vHallSens_handler.new_MtrRPM = 0.f;
			vHallSens_handler.zeroSpd_cnt = 0;
		}
	}
	else
	{
		vHallSens_handler.zeroSpd_cnt = 0;
	}

	vHallSens_handler.old_MtrRPM = vHallSens_handler.new_MtrRPM;
}

float hallsens_get_motorRPM(void)
{
    return vHallSens_handler.new_MtrRPM;
}
