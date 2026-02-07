/*--------------------------------------*/
// Author   : sungsoo
// Date     : 26.01.14
// Name     : tasksch.c
/*--------------------------------------*/

/*===== tasksch.c =====*/

#include "main.h"
#include "tasksch.h"
#include <stdbool.h>
#include <stdint.h>

#pragma pack(push, 1)

typedef struct TaskInfo
{
    // identification info
    uint8_t taskIdx;
    void (*taskFunc_ptr)(void);

    // scheduling info
    uint16_t period_ms;
    uint16_t offset_ms;
    volatile uint32_t curTick_ms;
    volatile bool exec_flag;

    // major cycle related info
    volatile bool cycledOnce_flag;

    // runtime error
    volatile uint16_t indiOverRun_totalCnt; 
    uint16_t indiOverRun_cycleCnt; 
    uint32_t taskCnt;
} typTaskInfo;

typedef struct SchInfo
{
    // initialization info
    bool isInitialized;
    eInitErrCode initErrCode;
    eInitStatus initStatus;

    // runtime info
    eRunErrCode runErrCode;
    bool majorCycle_flag;
    uint8_t majorCycle_taskExecCnt;
    bool tasksch_Exit_Flag;
    uint16_t overRun_count;
} typSchInfo;

#pragma pack(pop)

static typExecTimer vTaskSch_totalExecCLK;
static typTaskInfo vTaskSch_taskList[TASKSCH_NUMBER];
static typSchInfo vTaskSch_info;

/* Task Scheduler - RunTime Assert */

bool tasksch_feedWatchdog(void);

static inline void tasksch_runTime_assertFunc(void)
{
    while (1)
    {
#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE)
		tasksch_feedWatchdog();
#endif
    }
}

/* Task Scheduler - Task Initialization */

static void tasksch_taskConfig(uint8_t taskIdx, uint16_t period_ms, uint16_t offset_ms, void (*taskFunc_ptr)(void))
{
    if (taskIdx < TASKSCH_NUMBER && taskFunc_ptr != NULL && period_ms > 0U)
    {
        vTaskSch_taskList[taskIdx].taskIdx = taskIdx;
        vTaskSch_taskList[taskIdx].taskFunc_ptr = taskFunc_ptr;

        vTaskSch_taskList[taskIdx].period_ms = period_ms;
        vTaskSch_taskList[taskIdx].offset_ms = offset_ms;
        vTaskSch_taskList[taskIdx].curTick_ms = 0;

        vTaskSch_taskList[taskIdx].exec_flag = false;
        vTaskSch_taskList[taskIdx].cycledOnce_flag = false;
        vTaskSch_taskList[taskIdx].taskCnt = 0;
        vTaskSch_taskList[taskIdx].indiOverRun_totalCnt = 0;
        vTaskSch_taskList[taskIdx].indiOverRun_cycleCnt = 0;
    }
    else
    {
        vTaskSch_info.initErrCode = TASKSCH_INIT_ERR_TASK_CONFIG_FAIL;
        TASKSCH_ASSERT_FUNC();
    }
}

static void tasksch_initTasks(void)
{
    uint8_t idx = 0;

    typUserRegiTaskObj userTaskInfo = {NULL, 0, 0};
    for (; idx < TASKSCH_NUMBER; idx++)
    {
        tasksch_getUserRegi_taskInfo(idx, &userTaskInfo);
        tasksch_taskConfig(idx, userTaskInfo.regiTaskPeriod_ms,
                           userTaskInfo.regiTaskOffset_ms, userTaskInfo.regiTaskFunc_ptr);
    }
}

/* Task Scheduler - Measuring Task Execution Time */

static inline void tasksch_startCycleMeasurement(void)
{
#if (TASKSCH_CYCLE_MEASURING == TASKSCH_CYCLE_MEASURE_ENABLE)
    TASKSCH_WRITE_GPIO_PIN(TASKSCH_MEASURE_GPIO_PORT, TASKSCH_MEASURE_GPIO_PIN, TASKSCH_START_CYCLE_STAT);
#endif
    return;
}

static inline void tasksch_endCycleMeasurement(void)
{
#if (TASKSCH_CYCLE_MEASURING == TASKSCH_CYCLE_MEASURE_ENABLE)
    TASKSCH_WRITE_GPIO_PIN(TASKSCH_MEASURE_GPIO_PORT, TASKSCH_MEASURE_GPIO_PIN, TASKSCH_END_CYCLE_STAT);
#endif
    return;
}

/* Task Scheduler - __weak Function (userHook,Exit) */

__weak void tasksch_userInitCmpltHook(void) { return; }
__weak void tasksch_userFeedFail_watchdogHook(void) { return; }
__weak void tasksch_userMajorCycleHook(void) { return; }
__weak void tasksch_user_preExit_schedulerHook(void) { return; }
__weak void tasksch_userOverRun_thrshldExceedHook(void) { return; }

__weak bool tasksch_requestExit(void) { return false; }

/* Task Scheduler - Watchdog Management */

__weak bool tasksch_initWatchdog(void) { return true; }
__weak bool tasksch_beginWatchdog(void) { return true; }
__weak bool tasksch_feedWatchdog(void) { return true; }

/* Task Scheduler - Major Cycle Event Functions */

static bool tasksch_isMajorCycle(void)
{
    if (vTaskSch_info.majorCycle_taskExecCnt == TASKSCH_NUMBER)
    {
        vTaskSch_info.majorCycle_taskExecCnt = 0;
        return true;
    }
    else
    {
        return false;
    }
}

static void tasksch_update_majorCycle_infos(void)
{
    for (uint8_t i = 0; i < TASKSCH_NUMBER; i++)
    {
        vTaskSch_taskList[i].cycledOnce_flag = false;
        TASKSCH_DISABLE_ISR();
        vTaskSch_info.overRun_count += vTaskSch_taskList[i].indiOverRun_cycleCnt;
        vTaskSch_taskList[i].indiOverRun_totalCnt += vTaskSch_taskList[i].indiOverRun_cycleCnt;
		vTaskSch_taskList[i].indiOverRun_cycleCnt = 0;
        TASKSCH_ENABLE_ISR();
    }
}

static void tasksch_majorCycleEvent(void)
{
#if (TASKSCH_MAJOR_CYCLE_HOOK == TASKSCH_MAJOR_CYCLE_HOOK_ENABLE)
    // User-defined idle hook function
    tasksch_userMajorCycleHook();
#endif
    // GPIO set
    tasksch_startCycleMeasurement();

    // Feed the watchdog
    if (tasksch_feedWatchdog() == false)
    {
        vTaskSch_info.runErrCode = TASKSCH_RUN_ERR_WATCHDOG_FAIL;
        tasksch_userFeedFail_watchdogHook();
        TASKSCH_ASSERT_FUNC();
    }
    else
    {
        // Reset cycled flags
        tasksch_update_majorCycle_infos();
        // GPIO reset
        tasksch_endCycleMeasurement();
        vTaskSch_info.majorCycle_flag = false;
    }

    return;
}

static void tasksch_initEvent(void)
{
#if (TASKSCH_INIT_HOOK == TASKSCH_INIT_HOOK_ENABLE)
    // User-defined init hook function
    // User can override this function in tasksch_config.c
    tasksch_userInitCmpltHook();
#endif

    vTaskSch_info.isInitialized = true;
    vTaskSch_info.initStatus = TASKSCH_INIT_COMPLETED;
    return;
}

/* Task Scheduler - Execution Clock Management */

static void tasksch_initExecClock(void)
{
    vTaskSch_totalExecCLK.day = 0;
    vTaskSch_totalExecCLK.hour = 0;
    vTaskSch_totalExecCLK.min = 0;
    vTaskSch_totalExecCLK.sec = 0;
    vTaskSch_totalExecCLK.mili_sec = 0;
}

static void tasksch_updateExecClock(void) // execute in 1ms timer interrupt
{
    vTaskSch_totalExecCLK.mili_sec++;

    if (vTaskSch_totalExecCLK.mili_sec >= 1000)
    {
        vTaskSch_totalExecCLK.mili_sec = 0;
        vTaskSch_totalExecCLK.sec++;
    }

    if (vTaskSch_totalExecCLK.sec >= 60)
    {
        vTaskSch_totalExecCLK.sec = 0;
        vTaskSch_totalExecCLK.min++;
    }

    if (vTaskSch_totalExecCLK.min >= 60)
    {
        vTaskSch_totalExecCLK.min = 0;
        vTaskSch_totalExecCLK.hour++;
    }

    if (vTaskSch_totalExecCLK.hour >= 24)
    {
        vTaskSch_totalExecCLK.hour = 0;
        vTaskSch_totalExecCLK.day++;
    }
}

typExecTimer tasksch_getCurrTime(void)
{
    typExecTimer tempTime;

    TASKSCH_DISABLE_ISR();
    tempTime = vTaskSch_totalExecCLK;
    TASKSCH_ENABLE_ISR();

    return tempTime;
}

/* Task Scheduler - Overrun Detection */

uint16_t tasksch_getOverRunCount(void)
{
    uint16_t tempOverrunCnt = 0;

    tempOverrunCnt = vTaskSch_info.overRun_count;

    return tempOverrunCnt;
}

static inline void tasksch_clearOverrunCount(void)
{
    vTaskSch_info.overRun_count = 0;
}

static inline void tasksch_checkOverrunThreshold(void)
{
#if (TASKSCH_OVERRUN_DETECT == TASKSCH_OVERRUN_DETECT_ENABLE)
    if (vTaskSch_info.overRun_count >= TASKSCH_OVERRUN_THRESHOLD_CNT)
    {
        vTaskSch_info.runErrCode = TASKSCH_RUN_ERR_OVERRUN_CNT_EXCEEDED;
#if (TASKSCH_OVERRUN_HOOK == TASKSCH_OVERRUN_HOOK_ENABLE)
        // User-defined action when overrun threshold is exceeded
        tasksch_userOverRun_thrshldExceedHook();
#endif
        TASKSCH_ASSERT_FUNC(); // induce watchdog reset or infinite loop
    }
#endif
    return;
}

static inline void tasksch_detectOverRun(typTaskInfo *task)
{
#if (TASKSCH_OVERRUN_DETECT == TASKSCH_OVERRUN_DETECT_ENABLE)

    if (task->exec_flag == true)
    {
        task->indiOverRun_cycleCnt++;
    }

#endif
    return;
}

/* Task Scheduler - Core Logic */

static void tasksch_infoInit(void)
{
    // init task scheduler info
    vTaskSch_info.isInitialized = false;
    vTaskSch_info.initErrCode = TASKSCH_INIT_ERR_OK;
    vTaskSch_info.initStatus = TASKSCH_INIT_NOT_STARTED;

    // runtime task scheduler info
    vTaskSch_info.runErrCode = TASKSCH_RUN_ERR_OK;
    vTaskSch_info.tasksch_Exit_Flag = false;
    vTaskSch_info.majorCycle_taskExecCnt = 0;
    vTaskSch_info.majorCycle_flag = false;
    vTaskSch_info.overRun_count = 0;
}

void tasksch_init(void)
{
    // Prevent re-initialization
    if (vTaskSch_info.isInitialized == true)
    {
        vTaskSch_info.runErrCode = TASKSCH_RUN_ERR_INIT_AGAIN;
        tasksch_runTime_assertFunc();
    }

    // Initialize task scheduler information
    tasksch_infoInit();
    vTaskSch_info.initStatus = TASKSCH_INIT_INFO_INITIALIZED;

    // Initialize user registration-only task information
    tasksch_init_RegiTaskObj();
    vTaskSch_info.initStatus = TASKSCH_INIT_REGI_TASK_CONFIGED;

    // Verifying user-defined registration-only task information
    if (tasksch_ValidateUser_RegiTaskInfos() == false)
    {
        vTaskSch_info.initErrCode = TASKSCH_INIT_ERR_TASK_INFO_INVALID;
        TASKSCH_ASSERT_FUNC();
    }
    else
    {
        vTaskSch_info.initStatus = TASKSCH_INIT_TASKS_VALIDATED;
    }

    // Task Initialization
    tasksch_initTasks();
    vTaskSch_info.initStatus = TASKSCH_INIT_ALL_TASKS_CONFIGURED;

    // Watchdog Initialization
    if (tasksch_initWatchdog() == false)
    {
        vTaskSch_info.initErrCode = TASKSCH_INIT_ERR_WATCHDOG_INIT_FAIL;
        TASKSCH_ASSERT_FUNC();
    }
    else
    {
        vTaskSch_info.initStatus = TASKSCH_INIT_WATCHDOG_INITIALIZED;
    }

    // Initialize system run time
    tasksch_initExecClock();
    vTaskSch_info.initStatus = TASKSCH_INIT_EXEC_CLOCK_INITIALIZED;

    // Watchdog Start
    if (tasksch_beginWatchdog() == false)
    {
        vTaskSch_info.initErrCode = TASKSCH_INIT_ERR_WATCHDOG_START_FAIL;
        TASKSCH_ASSERT_FUNC();
    }
    else
    {
        vTaskSch_info.initStatus = TASKSCH_INIT_WATCHDOG_STARTED;
    }

    // Done
    tasksch_initEvent();
}

void tasksch_execTask(void)
{
    bool exitRequested = false;

    if (vTaskSch_info.isInitialized == false)
    {
        vTaskSch_info.runErrCode = TASKSCH_RUN_ERR_INIT_NOT_DONE;
        TASKSCH_ASSERT_FUNC();
    }

    while (vTaskSch_info.tasksch_Exit_Flag == false)
    {
        tasksch_checkOverrunThreshold();

        for (uint8_t idx = 0; idx < TASKSCH_NUMBER; idx++)
        {

            if (vTaskSch_taskList[idx].exec_flag == true)
            {
                TASKSCH_DISABLE_ISR();
                vTaskSch_taskList[idx].exec_flag = false;
                TASKSCH_ENABLE_ISR();
                vTaskSch_taskList[idx].taskFunc_ptr();
                vTaskSch_taskList[idx].taskCnt++;

                if (vTaskSch_taskList[idx].cycledOnce_flag == false)
                {
                    // first time task executed in this major cycle
                    vTaskSch_info.majorCycle_taskExecCnt++;
                    vTaskSch_taskList[idx].cycledOnce_flag = true;
                }
            }
        }
        // check major cycle
        vTaskSch_info.majorCycle_flag = tasksch_isMajorCycle();

        if (vTaskSch_info.majorCycle_flag == true)
        {
            tasksch_majorCycleEvent();
            exitRequested = tasksch_requestExit();

            TASKSCH_DISABLE_ISR();
            vTaskSch_info.tasksch_Exit_Flag = exitRequested;
            TASKSCH_ENABLE_ISR();
        }
    }
#if (TASKSCH_PRE_EXIT_HOOK == TASKSCH_PRE_EXIT_HOOK_ENABLE)
    tasksch_user_preExit_schedulerHook();
#endif
}

void tasksch_timeManager(void)
{
    uint8_t idx = 0;

    if (vTaskSch_info.tasksch_Exit_Flag == true || vTaskSch_info.isInitialized == false)
        return;

    for (; idx < TASKSCH_NUMBER; idx++)
    {
        if (vTaskSch_taskList[idx].offset_ms > 0)
        {
            vTaskSch_taskList[idx].offset_ms--;
        }
        else
        {
            vTaskSch_taskList[idx].curTick_ms++;
            if (vTaskSch_taskList[idx].curTick_ms >= vTaskSch_taskList[idx].period_ms)
            {
                tasksch_detectOverRun(&vTaskSch_taskList[idx]); // detect overrun
                vTaskSch_taskList[idx].exec_flag = true;
                vTaskSch_taskList[idx].curTick_ms = 0;
            }
        }
    }

    tasksch_updateExecClock();
}
