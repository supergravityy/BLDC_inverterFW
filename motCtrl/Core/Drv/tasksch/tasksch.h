/*--------------------------------------*/
// Author   : sungsoo
// Date     : 26.01.14
// Name     : tasksch.h
/*--------------------------------------*/

/*===== tasksch.h =====*/

# pragma once

#include "main.h"
#include "tasksch_config.h"
#include <stdbool.h>
#include <stdint.h>

# pragma pack(push,1)

typedef struct execTimer
{
	uint8_t day;
	uint8_t hour;
	uint8_t min;
	uint8_t sec;
	uint16_t mili_sec;
}typExecTimer;

typedef enum InitStatus
{
	TASKSCH_INIT_NOT_STARTED = 0,
	TASKSCH_INIT_INFO_INITIALIZED,
	TASKSCH_INIT_REGI_TASK_CONFIGED,
	TASKSCH_INIT_TASKS_VALIDATED,
	TASKSCH_INIT_ALL_TASKS_CONFIGURED,
	TASKSCH_INIT_EXEC_CLOCK_INITIALIZED,
	TASKSCH_INIT_WATCHDOG_INITIALIZED,
	TASKSCH_INIT_WATCHDOG_STARTED,
	TASKSCH_INIT_COMPLETED
}eInitStatus;

typedef enum InitErrorCode
{
	TASKSCH_INIT_ERR_OK = 0,
	TASKSCH_INIT_ERR_TASK_INFO_INVALID,
	TASKSCH_INIT_ERR_TASK_CONFIG_FAIL,
	TASKSCH_INIT_ERR_WATCHDOG_INIT_FAIL,
	TASKSCH_INIT_ERR_WATCHDOG_START_FAIL
}eInitErrCode;

typedef enum runErrorCode
{
	TASKSCH_RUN_ERR_OK = 0,
	TASKSCH_RUN_ERR_INIT_AGAIN,
	TASKSCH_RUN_ERR_INIT_NOT_DONE,
	TASKSCH_RUN_ERR_WATCHDOG_FAIL,
	TASKSCH_RUN_ERR_OVERRUN_CNT_EXCEEDED
}eRunErrCode;

# pragma pack(pop)

/*---- get scheduler infos ----*/
extern typExecTimer tasksch_getCurrTime(void);
extern uint16_t tasksch_getOverRunCount(void);

/*---- scheduler control ----*/
extern void tasksch_init(void);
extern void tasksch_timeManager(void);
extern void tasksch_execTask(void);