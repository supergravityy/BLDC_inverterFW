#include "mtrCtrl.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "exti.h"
#include "../Inc/Gpio.h"
#include "tim.h"
#include "../hallsens/hallsens.h"
#include "../sensing/sensing.h"

#define MTRCTRL_PI_MAX_CCR_VAL 	(THROTTLE_CCR_MAXVAL) // PI 제어로 계산된 CCR 값이 최대 듀티 카운트 값을 넘지 않도록 제한
#define MTRCTRL_PI_PERIOD		(MTRCTRL_PI_CTRL_MS / 1000.f)

typMtrCtrl_manager vMotorCtrl_manager;
typMtrCtrl_handle_byPI vPiCtrl_handler;

extern typThrottle_handle vThrottle_handler; // sensing.c의 스로틀 핸들러 객체 참조

void mtrCtrl_PI_clearTerms(void)
{
    vPiCtrl_handler.P_term = 0.0f;
    vPiCtrl_handler.I_term = 0.0f;
    vPiCtrl_handler.PI_term = 0.0f;
    vPiCtrl_handler.rpm_rampVal = 0.f;
    vPiCtrl_handler.CCR_refVal = 0.f;
}

void mtrCtrl_PI_setTunings(float Kp, float Ki)
{
    vPiCtrl_handler.Kp = Kp;
    vPiCtrl_handler.Ki = Ki;
}

void mtrCtrl_PI_setRPMRef(float rpm_ref)
{
    vPiCtrl_handler.rpm_refVal = rpm_ref;
}

static void mtrCtrl_PI_init(float Kp, float Ki)
{
    // PI 제어에 필요한 변수 초기화

	mtrCtrl_PI_setTunings(Kp, Ki);

	mtrCtrl_PI_setRPMRef(0.0);

    mtrCtrl_PI_clearTerms();
}

float mtrCtrl_PI_getRPMRef(void)
{
    return vPiCtrl_handler.rpm_refVal;
}

// NOTE : 지령주었을 때 정상상태 도달까지의 시간 100ms, 속도리플 5% 미만, 오버슛/언더슛 제한 10%

void mtrCtrl_PI_update(void)
{
	float currRPM = hallsens_get_motorRPM();

	if((mtrCtrl_getSelCtrlMode() == MTRCTRL_CTRL_PI) && (throttle_get_validateFlg() == true))
	{
        // 듀티값이 순간적으로 튀는 걸 방지하기 위해, 지령값에 ramp처리를 한다.
        utils_ramp2Tgt(vPiCtrl_handler.rpm_refVal, &vPiCtrl_handler.rpm_rampVal, MTRCTRL_PI_RAMP_MAXVAL);

		// vPiCtrl_handler.rpm_error = vPiCtrl_handler.rpm_refVal - currRPM;
        vPiCtrl_handler.rpm_error = vPiCtrl_handler.rpm_rampVal - currRPM;

		vPiCtrl_handler.P_term = vPiCtrl_handler.Kp * vPiCtrl_handler.rpm_error;
		vPiCtrl_handler.I_term += vPiCtrl_handler.Ki * vPiCtrl_handler.rpm_error * MTRCTRL_PI_PERIOD;

        // I-clamping
		if(vPiCtrl_handler.I_term > MTRCTRL_PI_MAX_CCR_VAL)		vPiCtrl_handler.I_term = MTRCTRL_PI_MAX_CCR_VAL;
		else if (vPiCtrl_handler.I_term < 0)					vPiCtrl_handler.I_term = 0;

		vPiCtrl_handler.PI_term = vPiCtrl_handler.P_term + vPiCtrl_handler.I_term;

        // PI-clamping
		if(vPiCtrl_handler.PI_term > MTRCTRL_PI_MAX_CCR_VAL)	vPiCtrl_handler.PI_term = MTRCTRL_PI_MAX_CCR_VAL;
		else if (vPiCtrl_handler.PI_term < 0) 					vPiCtrl_handler.PI_term = 0;

		vPiCtrl_handler.CCR_refVal = (uint32_t)vPiCtrl_handler.PI_term;
	}
	else
	{
		mtrCtrl_PI_clearTerms();
		vPiCtrl_handler.rpm_rampVal = currRPM;
	}
}

void mtrCtrl_objInit(float Kp, float Ki)
{
    mtrCtrl_PI_init(Kp, Ki);

    vMotorCtrl_manager.selCtrl_mode = MTRCTRL_CTRL_THROTTLE;
    vMotorCtrl_manager.throttleCtrl = &vThrottle_handler;
    vMotorCtrl_manager.piCtrl = &vPiCtrl_handler;
    vMotorCtrl_manager.final_CCR_refVal = 0;

    vMotorCtrl_manager.motor_RPM = 0.0f;
    vMotorCtrl_manager.overCurr_cnt = 0;

    vMotorCtrl_manager.motor_speed_KMH = 0.0f;
    vMotorCtrl_manager.errCode = MTRCTRL_ERR_NONE;
    vMotorCtrl_manager.thrttl_Ctrl = false;
    vMotorCtrl_manager.peripheral_init = false;
}

void mtrCtrl_calc_mtrSpeed(void)
{
	vMotorCtrl_manager.motor_RPM = hallsens_get_motorRPM();
    vMotorCtrl_manager.motor_speed_KMH = UTILS_RPM_TO_KMH(vMotorCtrl_manager.motor_RPM);
}

float mtrCtrl_getMotorSpeed_KMH(void)
{
    return vMotorCtrl_manager.motor_speed_KMH;
}

void mtrCtrl_setFinalCCR_refVal(void)
{
	typMtrCtrl_selCtrl_mode mode = mtrCtrl_getSelCtrlMode();

    if (mode == MTRCTRL_CTRL_THROTTLE)
    {
        vMotorCtrl_manager.final_CCR_refVal = vMotorCtrl_manager.throttleCtrl->CCR_refVal;
    }
    else if (mode == MTRCTRL_CTRL_PI)
    {
        vMotorCtrl_manager.final_CCR_refVal = vMotorCtrl_manager.piCtrl->CCR_refVal;
    }
}

uint32_t mtrCtrl_getFinalCCR_refVal(void)
{
    return vMotorCtrl_manager.final_CCR_refVal;
}

typMtrCtrl_selCtrl_mode mtrCtrl_getSelCtrlMode(void)
{
    return vMotorCtrl_manager.selCtrl_mode;
}

void mtrCtrl_setSelCtrlMode(typMtrCtrl_selCtrl_mode mode)
{
    vMotorCtrl_manager.selCtrl_mode = mode;
}

bool mtrCtrl_getPeriphInit(void)
{
    return vMotorCtrl_manager.peripheral_init;
}

void mtrCtrl_setPeriphInit(void)
{
    vMotorCtrl_manager.peripheral_init = true;
}

void mtrCtrl_setCtrlContinue(bool isContinue)
{
    vMotorCtrl_manager.thrttl_Ctrl = isContinue;
}

bool mtrCtrl_getCtrlContinue(void)
{
    return vMotorCtrl_manager.thrttl_Ctrl;
}

// todo : 나중에 에러의 우선순위에 맞춰서 동작하게 수정하기
void mtrCtrl_chkErrSt(typMtrCtrl_errCode setErr) // 입력 : 어떤 에러체크를 해야하는지
{
    switch (setErr)
    {
    case MTRCTRL_ERR_NONE:
        // do nothing
        break;
    case MTRCTRL_ERR_OVER_CURRENT:

        if (sensingCurr_getOverCurrent_st() == true) // 과전류 상태가 실제로 감지될 때만 에러 코드 기록
        {
            vMotorCtrl_manager.overCurr_cnt++;

            if (vMotorCtrl_manager.overCurr_cnt >= MTRCTRL_OVERCURR_MAXCNT)
            {
                vMotorCtrl_manager.errCode = MTRCTRL_ERR_OVER_CURRENT;
            }
        }
        else if (vMotorCtrl_manager.errCode == MTRCTRL_ERR_OVER_CURRENT)
        {
            vMotorCtrl_manager.errCode = MTRCTRL_ERR_OVER_CURRENT; // 이미 과전류 에러가 기록되어 있으면 유지 (과전류 상태가 지속되고 있다고 판단)
        }
        break;

    case MTRCTRL_ERR_MOS_HOT:

		if (NTC_getHeat_st() == true)
			vMotorCtrl_manager.errCode = MTRCTRL_ERR_MOS_HOT; // 모스 과열 에러는 가장 심각하므로 무조건 기록
		else if (vMotorCtrl_manager.errCode == MTRCTRL_ERR_MOS_HOT)
			vMotorCtrl_manager.errCode = MTRCTRL_ERR_NONE; // 과열이 감지되지 않으면 에러 해제

        /*if (vMotorCtrl_manager.errCode > MTRCTRL_ERR_MOS_HOT)
        {
            if (NTC_getHeat_st() == true)
                vMotorCtrl_manager.errCode = MTRCTRL_ERR_MOS_HOT; // 모스 과열 에러는 가장 심각하므로 무조건 기록
            else if (vMotorCtrl_manager.errCode == MTRCTRL_ERR_MOS_HOT)
                vMotorCtrl_manager.errCode = MTRCTRL_ERR_NONE; // 과열이 감지되지 않으면 에러 해제
        }*/
        break;

    case MTRCTRL_ERR_UNDER_VOLT:

    	if(dcVolt_getLowVolt_st() == true)
    		vMotorCtrl_manager.errCode = MTRCTRL_ERR_UNDER_VOLT;
    	else
    		vMotorCtrl_manager.errCode = MTRCTRL_ERR_NONE;

        /*if (vMotorCtrl_manager.errCode > MTRCTRL_ERR_UNDER_VOLT)
        {
            if (dcVolt_getLowVolt_st() == true)
                vMotorCtrl_manager.errCode = MTRCTRL_ERR_UNDER_VOLT; // 저전압 에러는 과전압보다 심각하므로 덮어쓰기 허용
            else
                vMotorCtrl_manager.errCode = MTRCTRL_ERR_NONE; // 저전압이 감지되지 않으면 에러 해제
        }*/
        break;

    case MTRCTRL_ERR_OVER_VOLT:
        // todo : 나중에 과전압 에러 체크 로직 추가 (예: dcVolt_getOverVolt_st() 함수 구현 후 호출)
        break;

    default:
        // do nothing
        break;
    }
}

typMtrCtrl_errCode mtrCtrl_getErrCode(void)
{
    return vMotorCtrl_manager.errCode;
}

bool mtrCtrl_getAppInit_flg(void)
{
    return vMotorCtrl_manager.app_init;
}

void mtrCtrl_setAppInit_flg(void)
{
    vMotorCtrl_manager.app_init = true;
}
