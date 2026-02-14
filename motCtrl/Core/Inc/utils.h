#pragma once 

#include "stm32f767xx.h"
#include <stdbool.h>

#ifndef USE_INWHEEL
#define USE_INWHEEL 1
#endif

/* ---------- math ---------- */ 

#ifndef PI
#define PI                              (3.14159f)
#endif

/* ---------- Mechanics ---------- */ 

// 바퀴 지름 [m] — 하드웨어에 맞게 수정
#ifndef WHEEL_DIAMETER_M
#define WHEEL_DIAMETER_M                (0.2000f) // 예: 8-inch ≈ 0.2032 m
#endif
#define WHEEL_CIRCUM_M                  (PI * (WHEEL_DIAMETER_M))

// Hall 엣지 수 (한 바퀴당 인터럽트 수
//소형 BLDC모터:24.0f, 인휠모터:90.0f로 수정
#ifndef HALL_EDGES_PER_REV
#ifdef USE_INWHEEL
#define HALL_EDGES_PER_REV              (90.0f)
#else
#define HALL_EDGES_PER_REV              (24.0f)
#endif
#endif

// 1: 시계방향(CW), 0: 반시계(CCW)
#ifndef MOTOR_DIR_CW
#define MOTOR_DIR_CW                    (1)
#define MOTOR_DIR_CCW                   (0)
#endif

#ifndef MOTOR_DIR
#define MOTOR_DIR MOTOR_DIR_CW
#endif

/* ---------- ADC & Sensors ---------- */ 
#define ADC_VREF                        (3.3f)
#define ADC_FS                          (4095.0f)
#define VDIV_RATIO                      (0.057362f) // Vdc 측정 저항분배 비율
#define OFFSET_Volt                     (1.65f)
#define OPAMP_GAIN                      (0.044f) // 전류센서 OPAMP 게인

/* ---------- Thresholds ---------- */ 
#define THROTTLE_OFF                    (1.00f)
#define THROTTLE_ON                     (1.05f)

#define OC_LEVEL                        (35.0f)
#define OC_TRIP_COUNT                   (50) // 2.5ms@20kHz 예시
#define UTILS_RPM_TO_KMH(rpm)                 ((rpm) * (WHEEL_CIRCUM_M) * 60.0f / 1000.0f)

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