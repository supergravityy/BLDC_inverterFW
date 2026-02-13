#pragma once 

#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"

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

#define THROTTLE_OFF_VOLT       (1.0f)
#define THROTTLE_ON_VOLT        (1.05f)
#define THROTTLE_RAMP_UNIT      (0.15f)
#define THROTTLE_ADC_TO_VOLT    (ADC_VREF / ADC_FS)

typedef struct throttle_handle
{
    // 센싱
    uint32_t rawVal;
    float procVal;
    bool val_is_validate; // 히스테리시스(on/off 기준 틀림)를 주어 데드존을 만들어 채터링 방지

    // 제어
    float refVal;
    float rampVal;
    uint32_t VoltageRef;
}typThrottle_handle;

uint32_t throttle_update_proc(void);
void throttle_init(void);