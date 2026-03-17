/*--------------------------------------*/
// Author   : sungsoo
// Date     : 26.01.14
// Name     : tasksch_config.c
/*--------------------------------------*/

/*===== tasksch_config.c =====*/

#include "main.h"
#include "tasksch_config.h"
#include <stdbool.h>

/*---- USER INCLUDE ----*/

#include "../../Inc/Gpio.h"
#include "../../Inc/uart.h"
#include "../hallSens/hallsens.h"
#include "../sensing/sensing.h"
#include "../mtrCtrl/mtrCtrl.h"
#include "../../Inc/iwdg.h"
#include "../sysinput/sysInput.h"
#include "string.h"

// NOTE : Declare user includes

#define VALIDATE_TASK_INFO(taskPtr, taskPeriod) ((taskPtr == NULL) || (taskPeriod == 0U))

#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE)
#ifdef TASKSCH_STM32_HAL_USE

static IWDG_HandleTypeDef hiwdg; // IWDG handle in STM32 HAL

#endif
#endif

static typUserRegiTaskObj vUserRegiTaskObj[TASKSCH_NUMBER];

/* Define your Task */

// INFO : The prototype of the function is "void function01(void)"

void Task_1ms(void)
{
	throttle_update_proc();
    mtrCtrl_PI_update();

    mtrCtrl_setFinalCCR_refVal();
}

void Task_10ms(void)
{
#if (MTR_INVTR_CTRL_MODE == MTR_INVTR_CTRL_APP)
    sysInput_parseData();
#endif

	mtrCtrl_calc_mtrSpeed();
    mtrCtrl_setSelCtrlMode(sysInput_getMode());
    mtrCtrl_setCtrlContinue(!sysInput_getPause());

    if(mtrCtrl_getSelCtrlMode() == MTRCTRL_CTRL_PI)
    {
    	mtrCtrl_PI_setTunings(sysInput_getKp(), sysInput_getKi());
    	mtrCtrl_PI_setRPMRef((float)sysInput_getRefRPM());
    }
}

void Task_10ms_2(void)
{
    if(mtrCtrl_getSelCtrlMode() == MTRCTRL_CTRL_THROTTLE)
    {
        uart_debug_sendStr_polling(">refVolt:", strlen(">refVolt:"));
        uart_debug_sendFloat_polling(throttle_get_refVolt(),2);
        uart_debug_sendStr_polling("\n", strlen("\n"));
    }
    else
    {
        uart_debug_sendStr_polling(">refRPM:", strlen(">refRPM:"));
        uart_debug_sendFloat_polling(mtrCtrl_PI_getRPMRef(),2);
        uart_debug_sendStr_polling("\n", strlen("\n"));
    }
}

void Task_10ms_4(void)
{
	uart_debug_sendStr_polling(">RPM:", strlen(">RPM:"));
	uart_debug_sendFloat_polling(hallsens_get_motorRPM(),1);
	uart_debug_sendStr_polling("\n", strlen("\n"));
}

void Task_10ms_6(void)
{
	uart_debug_sendStr_polling(">Imax:", strlen(">Imax:"));
	uart_debug_sendFloat_polling(sensing_getIphase_max(), 1);
	uart_debug_sendStr_polling("\n", strlen("\n"));
}

void Task_10ms_8(void)
{
	uart_debug_sendStr_polling(">refCCR:", strlen(">refCCR:"));
	uart_debug_sendInt_polling(mtrCtrl_getFinalCCR_refVal());
	uart_debug_sendStr_polling("\n", strlen("\n"));
}

void Task_100ms(void)
{
	mtrCtrl_chkErrSt(MTRCTRL_ERR_MOS_HOT);
	mtrCtrl_chkErrSt(MTRCTRL_ERR_UNDER_VOLT);
}

void Task_500ms(void)
{
    /* uart_AT09_sendStr_polling("Spd : ", strlen("Spd : "));
    uart_AT09_sendFloat_polling(mtrCtrl_getMotorSpeed_KMH(),2);

    uart_AT09_sendStr_polling("\r\nRPM : ", strlen("\r\nRPM : "));
    uart_AT09_sendFloat_polling(hallsens_get_motorRPM(),2);

    uart_AT09_sendStr_polling("\r\nVdc : ", strlen("\r\nVdc : "));
    uart_AT09_sendFloat_polling(dcVolt_voltage(),1);

    uart_AT09_sendStr_polling("\r\ntemp : ",strlen("\r\ntemp : "));
    uart_AT09_sendFloat_polling(NTC_getTemper(),1);

    uart_AT09_sendStr_polling("\r\nfaultNum : ", strlen("\r\nfaultNum : "));
    uart_AT09_sendFloat_polling(mtrCtrl_getErrCode(),1);

    uart_AT09_sendStr_polling("\r\n\n",strlen("\r\n\n")); */

	gpio_toggle_pin(FLT_LED_Port, 6);
}

void Task_1sec(void)
{

}

void tasksch_init_RegiTaskObj(void)
{
    /* hard-coded Registor task initialization */

    // 1. 1ms 핵심 제어 태스크
    vUserRegiTaskObj[0].regiTaskFunc_ptr = Task_1ms;
    vUserRegiTaskObj[0].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[0].regiTaskPeriod_ms = 1;

    // 2. 10ms 모터 상태 연산 태스크
    vUserRegiTaskObj[1].regiTaskFunc_ptr = Task_10ms;
    vUserRegiTaskObj[1].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[1].regiTaskPeriod_ms = 10;

    // 3. 10ms UART 분산 전송 태스크 (Phase: 2ms)
    vUserRegiTaskObj[2].regiTaskFunc_ptr = Task_10ms_2;
    vUserRegiTaskObj[2].regiTaskOffset_ms = 2;
    vUserRegiTaskObj[2].regiTaskPeriod_ms = 10;

    // 4. 10ms UART 분산 전송 태스크 (Phase: 4ms)
    vUserRegiTaskObj[3].regiTaskFunc_ptr = Task_10ms_4;
    vUserRegiTaskObj[3].regiTaskOffset_ms = 4;
    vUserRegiTaskObj[3].regiTaskPeriod_ms = 10;

    // 5. 10ms UART 분산 전송 태스크 (Phase: 6ms)
    vUserRegiTaskObj[4].regiTaskFunc_ptr = Task_10ms_6;
    vUserRegiTaskObj[4].regiTaskOffset_ms = 6;
    vUserRegiTaskObj[4].regiTaskPeriod_ms = 10;

    // 6. 10ms UART 분산 전송 태스크 (Phase: 8ms)
    vUserRegiTaskObj[5].regiTaskFunc_ptr = Task_10ms_8;
    vUserRegiTaskObj[5].regiTaskOffset_ms = 8;
    vUserRegiTaskObj[5].regiTaskPeriod_ms = 10;

    // 7. 100ms 에러 감시 태스크
    vUserRegiTaskObj[6].regiTaskFunc_ptr = Task_100ms;
    vUserRegiTaskObj[6].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[6].regiTaskPeriod_ms = 100;

    // 8. 500ms 시스템 상태 표시 (하트비트 LED)
    vUserRegiTaskObj[7].regiTaskFunc_ptr = Task_500ms;
    vUserRegiTaskObj[7].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[7].regiTaskPeriod_ms = 500;

    // 9. 1sec 시스템
    vUserRegiTaskObj[8].regiTaskFunc_ptr = Task_1sec;
    vUserRegiTaskObj[8].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[8].regiTaskPeriod_ms = 1000;
}

/* Override your __weak Functions */

// INFO : The function you want to create is defined in tasksch.c in the form of __weak.

/* __weak Function List 
* tasksch_user_preExit_schedulerHook
* tasksch_userMajorCycleHook
* tasksch_userInitCmpltHook
* tasksch_userOverRun_thrshldExceedHook
* tasksch_userFeedFail_watchdogHook
* tasksch_requestExit
*/

/* Define your Watchdog Functions */

void tasksch_userInitCmpltHook(void)
{
    // MCAL 상위 계층의 모듈들 전부 초기화
	adc_offsetCalib();
    utils_LPF_RPM_init();
    utils_LPF_phaseCurr_init();
    hallsens_init();
    sensing_objs_Init();
    sysInput_init();

    uart_debug_sendStr_polling("inverter start!\n\n", strlen("inverter start!\n\n"));

    mtrCtrl_setAppInit_flg();
}

void tasksch_user_preExit_schedulerHook(void)
{
    uart_debug_sendStr_polling("inverter end!\n", strlen("inverter end!\n"));
}

bool tasksch_requestExit(void)
{
    return sysInput_getTurnOff();
}

bool tasksch_initWatchdog(void)
{
    // User can override this function in tasksch_config.c
    bool result = false;

#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE) // just example how to write

    result = iwdg_init(IWDG_DIV_256, TASKSCH_WATCHDOG_TIMEOUT_CNT);


#else
    result = true;
#endif
    return result;
}

bool tasksch_beginWatchdog(void)
{
    // User can override this function in tasksch_config.c
    bool result = false;

#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE) // just example how to write
   result = iwdg_start();
#else
    result = true;
#endif
    return result;
}

bool tasksch_feedWatchdog(void)
{
    // User can override this function in tasksch_config.c
    bool result = false;

#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE) // just example how to write
    result = iwdg_feed();
#else
    result = true;
#endif
    return result;
}

/* User Registrated Task Object Initialization */
// INFO : Do not modify it below!!

bool tasksch_ValidateUser_RegiTaskInfos(void)
{
    bool result = true;
    for (uint8_t idx = 0; idx < TASKSCH_NUMBER; idx++)
    {
        if (VALIDATE_TASK_INFO(vUserRegiTaskObj[idx].regiTaskFunc_ptr, vUserRegiTaskObj[idx].regiTaskPeriod_ms) == true)
        {
            result = false;
            break;
        }
    }

    return result;
}

void tasksch_getUserRegi_taskInfo(uint8_t taskIdx, typUserRegiTaskObj *taskInfo)
{
    taskInfo->regiTaskFunc_ptr = vUserRegiTaskObj[taskIdx].regiTaskFunc_ptr;
    taskInfo->regiTaskPeriod_ms = vUserRegiTaskObj[taskIdx].regiTaskPeriod_ms;
    taskInfo->regiTaskOffset_ms = vUserRegiTaskObj[taskIdx].regiTaskOffset_ms;
}
