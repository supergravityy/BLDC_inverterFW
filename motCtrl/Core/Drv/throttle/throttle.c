#include "throttle.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "exti.h"
#include "../Inc/Gpio.h"
#include "../Inc/adc.h"

// 스로틀 전압을 pwm 듀티 카운트(CCR)로 변환 -> 엑셀파일의 표 참고
#define THROTTLE_RAW_VOLT_2_PWM_DUTY_CNT(volt)  (volt * 3400.f - 3540.f + 1000.f)

typThrottle_handle vThrottle_handler;

void throttle_init(void)
{
    vThrottle_handler.rawVal = 0;
    vThrottle_handler.procVal = 0;
    vThrottle_handler.val_is_validate = false;

    vThrottle_handler.refVal = 0;
    vThrottle_handler.rampVal = 0;
    vThrottle_handler.VoltageRef = 0;
}

static void throttle_sensing(void)
{
    // 1. adc로 스로틀 raw값 읽기
    vThrottle_handler.rawVal = adc_conv_rawThrottle_polling();

    // 2. adc raw값을 전압값으로 변환
    vThrottle_handler.procVal = (float)vThrottle_handler.rawVal * THROTTLE_ADC_TO_VOLT;

    // 3. adc 노이즈를 고려한 히스테리시스 처리
    if(vThrottle_handler.rawVal < THROTTLE_OFF_VOLT)
    {
        vThrottle_handler.val_is_validate = false;
    }
    else if(vThrottle_handler.rawVal > THROTTLE_ON_VOLT)
    {
        vThrottle_handler.val_is_validate = true;
    }
}

static void throttle_ramp2Tgt(float command, float *output, float ramp_unit)
// 한번에 너무 급격히 변화하지 않고 점진적으로 지령값에 도달하도록 처리하는 함수
{
    if(command > *output)
    {
        *output += ramp_unit;

        if(*output > command)   *output = command;
    }
    else if(command < *output)
    {
        *output -= ramp_unit;

        if(*output < command)   *output = command;
    }
    else
    {
        // do nothing
    }
}

static void throttle_postProcess(void)
{
    // 4. 스로틀 유효값 처리 및 제어신호 변환
    if(vThrottle_handler.val_is_validate == false)
    {
        vThrottle_handler.refVal = 0.0f;
    }
    else
    {
        vThrottle_handler.refVal = THROTTLE_RAW_VOLT_2_PWM_DUTY_CNT(vThrottle_handler.procVal);
    }

    // 5. 스로틀 램프함수 처리
    throttle_ramp2Tgt(vThrottle_handler.refVal, &vThrottle_handler.rampVal, THROTTLE_RAMP_UNIT);
}

// adc 값을 읽어서 사용하기 적절한 pwm신호로 출력하여 모터제어에 사용할 수 있게 함
uint32_t throttle_update_proc(void)
{
    throttle_sensing();
    throttle_postProcess();

    vThrottle_handler.VoltageRef = (uint32_t)(vThrottle_handler.rampVal);

    return vThrottle_handler.VoltageRef;
}