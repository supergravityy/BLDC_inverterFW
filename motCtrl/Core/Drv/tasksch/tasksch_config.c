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

}

void Task_10ms(void)
{

}

void Task_100ms(void)
{

}

void Task_500ms(void)
{

}

void Task_1sec(void)
{

}

void tasksch_init_RegiTaskObj(void)
{
    /* hard-coded Registor task initialization */

    vUserRegiTaskObj[0].regiTaskFunc_ptr = Task_1ms;
    vUserRegiTaskObj[0].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[0].regiTaskPeriod_ms = 1;

    vUserRegiTaskObj[1].regiTaskFunc_ptr = Task_10ms;
    vUserRegiTaskObj[1].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[1].regiTaskPeriod_ms = 10;

    vUserRegiTaskObj[2].regiTaskFunc_ptr = Task_100ms;
    vUserRegiTaskObj[2].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[2].regiTaskPeriod_ms = 100;

    vUserRegiTaskObj[3].regiTaskFunc_ptr = Task_500ms;
    vUserRegiTaskObj[3].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[3].regiTaskPeriod_ms = 500;

    vUserRegiTaskObj[4].regiTaskFunc_ptr = Task_1sec;
    vUserRegiTaskObj[4].regiTaskOffset_ms = 0;
    vUserRegiTaskObj[4].regiTaskPeriod_ms = 1000;
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

bool tasksch_initWatchdog(void)
{
    // User can override this function in tasksch_config.c
    bool result = false;

#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE) // just example how to write

#ifdef TASKSCH_STM32_HAL_USE
    result = true;
#endif
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

#ifdef TASKSCH_STM32_HAL_USE
    // Start the watchdog -> Initialize the IWDG peripheral (in HAL)

    hiwdg.Instance = IWDG;
    hiwdg.Init.Prescaler = TASKSCH_WATCHDOG_PRSCLAER_BITS;
    hiwdg.Init.Reload = TASKSCH_WATCHDOG_TIMEOUT_CNT; // 375 * 8ms = 3s

    result = (HAL_IWDG_Init(&hiwdg) != HAL_OK) ? false : true;
#endif

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
    #ifdef TASKSCH_STM32_HAL_USE
    // refresh the watchdog
    result = (HAL_IWDG_Refresh(&hiwdg) != HAL_OK) ? false : true;
    #endif
	
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
