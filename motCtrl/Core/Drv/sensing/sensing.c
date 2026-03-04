#include "sensing.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "exti.h"
#include "../Inc/Gpio.h"
#include "../Inc/adc.h"

// 스로틀 전압을 pwm 듀티 카운트(CCR)로 변환 -> 엑셀파일의 표 참고
#define THROTTLE_RAW_VOLT_2_PWM_DUTY_CNT(volt)  (volt * 3400.f - 3540.f + 1000.f)

// NTC raw값을 전압으로 변환 (예시, 실제로는 센서 특성에 따라 다름) -> 엑셀파일의 그래프 참고
#define NTC_VOLT_2_TEMPER(volt)  (-11.48f*volt*volt*volt+63.23*volt*volt-149.02*volt+181.97f) 

// 저항분배 비율고려 -> 회로도 참고
#define DCVOLT_VOLT_2_ACTUAL(volt)  (volt / UTILS_VDIV_RATIO)

#define SENSING_ADC_2_CURR(val) ((val * (UTILS_ADC_VREF / UTILS_ADC_MAXVAL)) / UTILS_OPAMP_GAIN)


typThrottle_handle vThrottle_handler;
typDcVolt_handle vDcVolt_handler;
typNTC_handle vNTC_handler;
typSensingCurr_handle vSensingCurr_handler;


void throttle_init(void)
{
    vThrottle_handler.rawVal = 0;
    vThrottle_handler.rawVolt_Val = 0;
    vThrottle_handler.val_is_validate = false;

    vThrottle_handler.refVal = 0;
    vThrottle_handler.rampVal = 0;
    vThrottle_handler.CCR_refVal = 0;
}

static void throttle_sensing(void)
{
    // 1. adc로 스로틀 raw값 읽기
    vThrottle_handler.rawVal = adc_conv_rawThrottle_polling();

    // 2. adc raw값을 전압값으로 변환
    vThrottle_handler.rawVolt_Val = (float)vThrottle_handler.rawVal * SENSING_ADC_2_VOLT;

    // 3. adc 노이즈를 고려한 히스테리시스 처리
    if(vThrottle_handler.rawVolt_Val < THROTTLE_OFF_VOLT)
    {
        vThrottle_handler.val_is_validate = false;
    }
    else if(vThrottle_handler.rawVolt_Val > THROTTLE_ON_VOLT)
    {
        vThrottle_handler.val_is_validate = true;
    }
}

static void throttle_postProcess(void)
{
    // 4. 스로틀 유효값 처리 및 제어신호 변환
    if(vThrottle_handler.val_is_validate == false)
    {
        vThrottle_handler.refVal = 0.0f;
        vThrottle_handler.rampVal = 0.0f;
    }
    else
    {
        vThrottle_handler.refVal = THROTTLE_RAW_VOLT_2_PWM_DUTY_CNT(vThrottle_handler.rawVolt_Val);
    }

    // 5. 스로틀 램프함수 처리
    utils_ramp2Tgt(vThrottle_handler.refVal, &vThrottle_handler.rampVal, THROTTLE_RAMP_UNIT);
}

// adc 값을 읽어서 사용하기 적절한 pwm신호로 출력하여 모터제어에 사용할 수 있게 함
void throttle_update_proc(void)
{
    uint32_t tempCCR_Val;
    throttle_sensing();
    throttle_postProcess();

    tempCCR_Val = (uint32_t)(vThrottle_handler.rampVal);

    // 마진 확보하여 최대 듀티 방지
    if(tempCCR_Val > THROTTLE_CCR_MAXVAL)
    {
        vThrottle_handler.CCR_refVal = THROTTLE_CCR_MAXVAL;
    }
    else
    {
        vThrottle_handler.CCR_refVal = tempCCR_Val;
    }
}

bool throttle_get_validateFlg(void)
{
	return vThrottle_handler.val_is_validate;
}

float throttle_get_refVolt(void)
{
    return vThrottle_handler.rawVolt_Val;
}

uint32_t throttle_get_CCR_ref(void)
{
    return vThrottle_handler.CCR_refVal;
}

void NTC_init(void)
{
    vNTC_handler.rawVal = 0;
    vNTC_handler.rawVolt_Val = 0;
    vNTC_handler.is_mosOverheat = false;
}

float NTC_getTemper(void)
{
    return vNTC_handler.temper;
}

bool NTC_getHeat_st(void)
{
    vNTC_handler.rawVal = adc_conv_rawNTC_polling();
    vNTC_handler.rawVolt_Val = (float)vNTC_handler.rawVal * SENSING_ADC_2_VOLT;
    vNTC_handler.temper = NTC_VOLT_2_TEMPER(vNTC_handler.rawVolt_Val );

    if(vNTC_handler.temper > NTC_OVERHEAT_CELCIUS)
    {
        vNTC_handler.is_mosOverheat = true;
    }
    else if(vNTC_handler.temper < NTC_NORMAL_CELCIUS)
    {
        vNTC_handler.is_mosOverheat = false;
    }

    return vNTC_handler.is_mosOverheat;
}

void dcVolt_init(void)
{
    vDcVolt_handler.rawVal = 0;
    vDcVolt_handler.volt = 0;
    vDcVolt_handler.is_lowVolt = false;
}

float dcVolt_voltage(void)
{
    return vDcVolt_handler.volt;
}

bool dcVolt_getLowVolt_st(void)
{
    vDcVolt_handler.rawVal = adc_conv_rawVdc_polling();
    vDcVolt_handler.volt = DCVOLT_VOLT_2_ACTUAL((float)vDcVolt_handler.rawVal * SENSING_ADC_2_VOLT);

    if(vDcVolt_handler.volt < DC_VOLT_LOW_THRESH)
    {
        vDcVolt_handler.is_lowVolt = true;
    }
    else
    {
        vDcVolt_handler.is_lowVolt = false;
    }

    return vDcVolt_handler.is_lowVolt;
}

void sensingCurr_init(void)
{
	uint32_t tempArr[IPHASE_CURR_NUM] = {0};
	adc_getOffsetCalib_val(tempArr);

    for(uint8_t i = 0; i < IPHASE_CURR_NUM; i++)
    {
        vSensingCurr_handler.Iphase[i].rawVal = 0;
        vSensingCurr_handler.Iphase[i].offset_rawVal = tempArr[i];
        vSensingCurr_handler.Iphase[i].rawCurr_A = 0;
    }
    vSensingCurr_handler.is_overCurrent = false;
    vSensingCurr_handler.Iphase_max = 0;
}

bool sensingCurr_getOverCurrent_st(void)
{
    float tempCalibVal;
    int32_t tempRawVal;
    // 1. adc로 전류 raw값 읽기
    vSensingCurr_handler.Iphase[IPHASE_U_IDX].rawVal = adc_conv_rawIus_polling();
    vSensingCurr_handler.Iphase[IPHASE_V_IDX].rawVal = adc_conv_rawIvs_polling();
    vSensingCurr_handler.Iphase[IPHASE_W_IDX].rawVal = adc_conv_rawIws_polling();

    // 2. raw값을 전류값으로 변환 (예시, 실제로는 센서 특성에 따라 다름)
    for(uint8_t i = 0; i < IPHASE_CURR_NUM; i++)
    {
        // 2.1 오프셋 보정
        tempRawVal = vSensingCurr_handler.Iphase[i].rawVal;
        tempCalibVal = (float)(tempRawVal - (int32_t)vSensingCurr_handler.Iphase[i].offset_rawVal); // 오프셋 보정
        
        // 2.2 전류값으로 변환
        vSensingCurr_handler.Iphase[i].rawCurr_A = SENSING_ADC_2_CURR(tempCalibVal);

        // 2.3 저역통과필터 적용
        vSensingCurr_handler.Iphase[i].filteredCurr_A = utils_LPF_phaseCurr_filter(vSensingCurr_handler.Iphase[i].rawCurr_A);
    }

    // 3. 최대전류 계산 (방향을 고려하지 않고 절대값으로 최대 전류 계산)
    vSensingCurr_handler.Iphase_max = UTILS_FIND_MAX_VAL(
        UTILS_ABS(vSensingCurr_handler.Iphase[IPHASE_U_IDX].filteredCurr_A),
        UTILS_ABS(vSensingCurr_handler.Iphase[IPHASE_V_IDX].filteredCurr_A),
        UTILS_ABS(vSensingCurr_handler.Iphase[IPHASE_W_IDX].filteredCurr_A)
    );

    // 4. 과전류 판단
    if(vSensingCurr_handler.Iphase_max > IPHASE_OVERCURR_THRESH)
    {
        vSensingCurr_handler.is_overCurrent = true;
    }
    else if(vSensingCurr_handler.Iphase_max < IPHASE_NORMAL_THRESH)
    {
        vSensingCurr_handler.is_overCurrent = false;
    }
    else
    {
        // do nothing
    }

    return vSensingCurr_handler.is_overCurrent;
}

float sensing_getIphase_max(void)
{
	return vSensingCurr_handler.Iphase_max;
}

void sensing_objs_Init(void)
{
    throttle_init();
    NTC_init();
    dcVolt_init();
    sensingCurr_init();
}
