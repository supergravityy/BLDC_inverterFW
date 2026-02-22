#pragma once

#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "exti.h"
#include "tim.h"
#include "../mtrCtrl/mtrCtrl.h"

#define HALLSENS_PH_NUM             (MTRCTRL_PHASE_CURR_NUM)
#define HALLSENS_U_IDX              (MTRCTRL_PHASE_CURR_U_IDX)
#define HALLSENS_V_IDX              (MTRCTRL_PHASE_CURR_V_IDX)    
#define HALLSENS_W_IDX              (MTRCTRL_PHASE_CURR_W_IDX)
#define HALLSENS_DUTY_MAX           (PWM1_PERIOD_TICK)

typedef enum mtrPhase
{
    MTR_DIR_REV = -1,
    MTR_DIR_FWD = 1,
    MTR_DIR_STP = 0
} typMtrPhase;

typedef struct hall_handle
{
    GPIO_TypeDef* port[HALLSENS_PH_NUM];
    uint8_t       pin[HALLSENS_PH_NUM];

    // 홀센서 위치측정
    uint8_t hallPin_state[HALLSENS_PH_NUM];   // 각 홀 센서의 현재 상태 (0 또는 1)
    uint32_t dutyVal[HALLSENS_PH_NUM];
    uint8_t curr_hallSum;                        // 1~6 사이의 HallSum 값
    uint8_t prev_hallSum;
    
    // 속도측정
    volatile uint32_t prevTick;
    volatile uint32_t currTick;
    volatile uint32_t deltaTick;
    volatile float rawMtrRPM;
    float filteredMtrRPM;

    bool is_initialized;
}typHall_Handle;

void hallsens_update_hallSeq(void);
void hallsens_init(void);
void hallsens_update_swtPattern(void);
void hallsens_cal_motorRPM(void);
float hallsens_get_motorRPM(void);
