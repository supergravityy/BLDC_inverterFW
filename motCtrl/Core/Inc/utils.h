#pragma once 

#include "stm32f767xx.h"
#include <stdbool.h>

#define MTR_INVTR_CTRL_DEBUG    (0UL)
#define MTR_INVTR_CTRL_APP      (1UL)
#define MTR_INVTR_CTRL_MODE     (MTR_INVTR_CTRL_APP)
/*
m>(str)     : 제어모드 변경
r>(int)     : 지령값 전달
p>(float)   : p게인 전달
i>(float)   : i게인 전달
t>          : 제어 일시중단
q>          : 시스템 종료
*/

/* ---------- math ---------- */ 
#define PI                              (3.14159f)

/* ---------- Mechanics ---------- */ 

// 바퀴 지름 [m] — 하드웨어에 맞게 수정
#define UTILS_WHEEL_DIAMETER_M          (0.2000f) // 예: 8-inch ≈ 0.2032 m
#define UTILS_WHEEL_CIRCUM_M            (PI * (UTILS_WHEEL_DIAMETER_M))

// Hall 엣지 수 (한 바퀴당 인터럽트 수)
#define UTILS_HALL_EDGES_PER_REV        (24.0f) // 8(센서) * 3(상)

// 1: 시계방향(CW), 0: 반시계(CCW)
#define UTILS_MOTOR_DIR_CW              (1)
#define UTILS_MOTOR_DIR_CCW             (0)
#define UTILS_MOTOR_DIR                 (UTILS_MOTOR_DIR_CW)

/* ---------- ADC & Sensors ---------- */ 
#define UTILS_ADC_VREF                  (3.3f)
#define UTILS_ADC_MAXVAL                (4095.0f)
#define UTILS_VDIV_RATIO                (0.057362f) // Vdc 측정 저항분배 비율
#define UTILS_OFFSET_Volt               (1.65f)
#define UTILS_OPAMP_GAIN                (0.044f) // 전류센서 OPAMP 게인

/* ---------- Thresholds ---------- */ 
#define UTILS_THRTTL_OFF                (1.00f)
#define UTILS_THRTTL_ON                 (1.05f)

#define UTILS_OVERCURR_LVL              (35.0f)
#define UTILS_RPM_TO_KMH(rpm)           ((rpm) * (UTILS_WHEEL_CIRCUM_M) * 60.0f / 1000.0f)

/* ---------- LPF ---------- */ 
#define UTILS_SAMPLING_TIME_SEC         (0.00005f)  // 20kHz 샘플링 기준 (PWM 주기-> center-aligned PWM)
#define UTILS_PHASE_CURR_CUTOFF_HZ      (500.0f)    // 상 전류 LPF 컷오프 주파수
#define UTILS_RPM_CUTOFF_HZ             (1.0f)      // RPM LPF 컷오프 주파수

/* ---------- SYS ---------- */ 
#define UTILS_F_CLK_MZ                  (216UL)    


/* ---------- REGISTER ---------- */ 
#define UTILS_BIT_SHIFT(pos,val)        (val << pos)
#define UTILS_BIT_MASK(pos,len)         (((1UL << (len)) - 1) << (pos)) // 시작비트 포지션에서 원하는 크기만큼 마스크를 만듬
// 비트를 길이만큼 왼쪽으로 밀고, 1을 빼서 하위비트들을 전부 1로만듬 -> 이 비트들을 다시 시프팅함

#define UTILS_FIND_MAX_VAL(a, b, c)     (((a) > (b)) ? (((a) > (c)) ? (a) : (c)) : (((b) > (c)) ? (b) : (c)))
#define UTILS_ABS(x)                    (((x) < 0) ? (-x) : (x))

#define UTILS_DISABLE_ISR()             __disable_irq()
#define UTILS_ENABLE_ISR()              __enable_irq()
#define UTILS_ASSERT_FUNC()             do{while(1);}while(0)

typedef struct lpf_handle
{
    float cutoff_hz;
    float prev_out;
    float Fx_coeff;
    
    bool is_1stRun; // 초기 응답성 확보
    bool is_initialized;
}typLpf_handle;

void utils_LPF_RPM_init(void);
void utils_LPF_phaseCurr_init(void);
float utils_LPF_RPM_filter(float input);
float utils_LPF_phaseCurr_filter(float input);

uint16_t utils_ipol_u16u16(const uint16_t* mapX, const uint16_t* mapY, uint16_t mapSize, uint16_t input);
uint32_t utils_ipol_u16u32(const uint16_t* mapX, const uint32_t* mapY, uint16_t mapSize, uint16_t input);
int16_t utils_ipol_s16s16(const int16_t* mapX, const int16_t* mapY, uint16_t mapSize, int16_t input);
int16_t utils_ipol_s16u16(const int16_t* mapX, const uint16_t* mapY, uint16_t mapSize, int16_t input);
int16_t utils_ipol_u32s16(const uint32_t* mapX, const int16_t* mapY, uint16_t mapSize, uint32_t input);
void utils_ramp2Tgt(float command, float *output, float maxUnit);