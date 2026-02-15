#pragma once 

#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "../sensing/sensing.h"

/* --- Motor Control --- */
#define MTRCTRL_PHASE_CURR_NUM       (3UL)
#define MTRCTRL_PHASE_CURR_U_IDX     (0UL)
#define MTRCTRL_PHASE_CURR_V_IDX     (1UL)
#define MTRCTRL_PHASE_CURR_W_IDX     (2UL)

// RPM 계산 타임아웃 판단 기준 카운트 (예시값, 실제로는 시스템 주기와 요구사항에 따라 조정 필요)
#define MTRCTRL_RPM_TIMEOUT_MAXCNT   (20000UL) // 1sec 동안 확인 
#define MTRCTRL_OVERCURR_MAXCNT      (1000UL) // 50ms 동안 확인

#define MTRCTRL_PI_CTRL_MS           (1)
#define MTRCTRL_PI_CTRL_PERIOD_SEC   ((float)(MTRCTRL_PI_CTRL_MS) / 1000.0f)
#define MTRCTRL_PI_iTERM_MAXVAL      ((float)(THROTTLE_PWM_PERIOD_VAL) * .4f) // PI 제어의 적분항 최대값 제한

typedef enum motorCtrl_errCode // 위험한 순으로 정렬 (에러 코드가 높을수록 더 심각한 에러로 판단)
{
    MTRCTRL_ERR_OVER_CURRENT = 1,
    MTRCTRL_ERR_MOS_HOT,
    MTRCTRL_ERR_OVER_VOLT,
    MTRCTRL_ERR_UNDER_VOLT,
    MTRCTRL_ERR_RPM_CALC_TIMEOUT,
    MTRCTRL_ERR_NONE
} typMtrCtrl_errCode;

typedef enum motorCtrl_selectedCtrl
{
    MTRCTRL_CTRL_THROTTLE = 0,
    MTRCTRL_CTRL_PI
} typMtrCtrl_selCtrl_mode;

typedef struct motorCtrl_byPI
{
    // PI 제어에 필요한 멤버변수들

    float Kp;
    float Ki;

    float rpm_error;
    float rpm_refVal;

    float P_term;
    float I_term;
    float PI_term;

    uint32_t CCR_refVal; // PI 제어로 계산된 최종 지령값 (예: PWM 듀티 카운트)
}typMtrCtrl_handle_byPI;

typedef struct motorCtrl_manager
{
    // 제어모드 객체
    typMtrCtrl_selCtrl_mode selCtrl_mode;
    typThrottle_handle* throttleCtrl;
    typMtrCtrl_handle_byPI* piCtrl;
    uint32_t final_CCR_refVal; // 모터제어에 필요한 최종 지령값 (예: PWM 듀티 카운트)

    // 모터 오류처리에 필요한 변수들
    float new_rpm;              // rpm 데이터 타임아웃 감지용
    float old_rpm;
    uint16_t rpm_timeout_cnt;
    uint16_t overCurr_cnt;      // 과전류 보호 카운터
    
    // 상태관리
    float motor_speed_KMH;
    typMtrCtrl_errCode errCode;
    bool    ctrlContinue_debug;
    bool    calib_cmplt;
    bool    app_init;
} typMtrCtrl_manager;


/* --- 초기화 및 설정 관련 --- */
void mtrCtrl_objInit(float Kp, float Ki);
void mtrCtrl_PI_setTunings(float Kp, float Ki);
void mtrCtrl_PI_setRPMRef(float rpm_ref);
void mtrCtrl_PI_clearTerms(void);

/* --- 제어 루프 업데이트 관련 --- */
void mtrCtrl_PI_update(void);
void mtrCtrl_setFinalCCR_refVal(void);
void mtrCtrl_calc_mtrSpeed(void);

/* --- 상태 및 에러 관리 관련 --- */
void mtrCtrl_setErrCode(typMtrCtrl_errCode setErr);
typMtrCtrl_errCode mtrCtrl_getErrCode(void);
bool mtrCtrl_check_RPM_timeout(void);

/* --- Getter / Setter (데이터 인터페이스) --- */
uint32_t mtrCtrl_getFinalCCR_refVal(void);
float mtrCtrl_getMotorSpeed_KMH(void);

typMtrCtrl_selCtrl_mode mtrCtrl_getSelCtrlMode(void);
void mtrCtrl_setSelCtrlMode(typMtrCtrl_selCtrl_mode mode);

bool mtrCtrl_isCalibCmplt(void);
void mtrCtrl_CalibCmplt(void);

void mtrCtrl_setCtrlContinue(bool isContinue);
bool mtrCtrl_getCtrlContinue(void);

bool mtrCtrl_getAppInit_flg(void);
void mtrCtrl_setAppInit_flg(void);