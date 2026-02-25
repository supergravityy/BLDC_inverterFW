#pragma once 

#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "tim.h"
#include "../../Inc/adc.h"

/*
* 스로틀 객체에 필요한 부분들
* 
*   멤버 변수
*   raw값, 변환값, 스로틀 출력여부,
*   
*   레퍼런스 값, (모터(pwm)와 스로틀의 관계 (엑셀파일 그래프))
*   가감속제어 처리후 값, (급가감속 제어)
*   최종지령 값, (이를)
* 
*   정의
*   채터링 방지용 히스테리스 off/on
*   ramp함수 통과용 유닛
*/

#define SENSING_ADC_2_VOLT      (UTILS_ADC_VREF / UTILS_ADC_MAXVAL)

#define THROTTLE_OFF_VOLT       (1.0f)
#define THROTTLE_ON_VOLT        (1.05f)
#define THROTTLE_RAMP_UNIT      (3.f)
#define THROTTLE_PWM_PERIOD_VAL (PWM1_PERIOD_TICK) // 타이머의 최대 카운트 값
#define THROTTLE_MAX_MARGIN     (100UL)
// 100% 듀티를 방지하기 위해 -> 하프브리지 구조에서 100%는 위험
#define THROTTLE_CCR_MAXVAL     (THROTTLE_PWM_PERIOD_VAL - THROTTLE_MAX_MARGIN)

#define NTC_OVERHEAT_CELCIUS    (100.0f) // NTC 과열 판단 기준 전압 (예시값, 실제로는 센서 특성에 따라 다름)
#define NTC_NORMAL_CELCIUS      (90.0f) // NTC 정상 판단 기준 전압 (예시값)

#define DC_VOLT_LOW_THRESH      (23.0f) // 저전압 판단 기준 전압

#define IPHASE_CURR_NUM         (3UL)
#define IPHASE_U_IDX            (0UL)
#define IPHASE_V_IDX            (1UL)
#define IPHASE_W_IDX            (2UL)
#define IPHASE_OVERCURR_THRESH  (35.0f) // 과전류 판단 기준 전류 (예시값)
#define IPHASE_NORMAL_THRESH    (34.0f) // 정상 판단 기준 전류 (예시값)

typedef struct throttle_handle
{
    // 센싱
    uint32_t rawVal;
    float rawVolt_Val;
    bool val_is_validate; // 히스테리시스(on/off 기준 틀림)를 주어 데드존을 만들어 채터링 방지

    // 제어
    float refVal;
    float rampVal;
    volatile uint32_t CCR_refVal;
}typThrottle_handle;

typedef struct NTC_handle
{
    uint32_t rawVal;
    float rawVolt_Val;
    float temper; // 온도값으로 변환된 NTC 센서 값
    bool is_mosOverheat; // MOSFET 과열 여부 판단
}typNTC_handle;

typedef struct dcVolt_handle
{
    uint32_t rawVal;
    float volt; // 전압값으로 변환된 DC 전압 센서 값
    bool is_lowVolt; // 저전압 여부 판단
}typDcVolt_handle;

typedef struct Iphase_handle
{
    uint32_t rawVal;
    uint32_t offset_rawVal; // 전류 오프셋 보정값
    float rawCurr_A;
    float filteredCurr_A; // 저역통과필터 적용된 전류값
}typIphase_handle;

typedef struct sensingCurr_handle
{
    typIphase_handle Iphase[IPHASE_CURR_NUM];

    float Iphase_max; // 과전류 판단 기준 전류 -> 위상차이기에 제일 큰 거 선택
    bool is_overCurrent;
}typSensingCurr_handle;

void throttle_update_proc(void);
bool throttle_get_validateFlg(void);
float throttle_get_refVolt(void);
uint32_t throttle_get_CCR_ref(void);

bool NTC_getHeat_st(void);
float NTC_getTemper(void);

float dcVolt_voltage(void);
bool dcVolt_getLowVolt_st(void);

void sensing_objs_Init(void);
bool sensingCurr_getOverCurrent_st(void);
