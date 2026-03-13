/*--------------------------------------*/
// Author   : sungsoo
// Date     : 26.01.14
// Name     : tasksch_config.h
/*--------------------------------------*/

/*===== tasksch_config.h =====*/

# pragma once

#include "main.h"
#include <stdbool.h>
#include <stdint.h>

/*---- USER INCLUDE ----*/

// NOTE : Declare user includes


// todo : By using "__weak", it is assumed that i'm using stm32.

//extern TIM_HandleTypeDef htim1;                                           // INFO : USER MUST DEFINE

/*---- REQUIRED SETTING LIST ----*/

//#define TASKSCH_STM32_HAL_USE // example for watchdog 

#define TASKSCH_NUMBER                      (9)                             // NOTE : USER DEFINE

#define TASKSCH_DISABLE_ISR()   		    __disable_irq();                // NOTE : USER DEFINE
#define TASKSCH_ENABLE_ISR()    		    __enable_irq();                 // NOTE : USER DEFINE

/*---- OPTIONALLY SETTING LIST ----*/

// INFO : Turn off this setting if long-period + long-term tasks → Overrun counts accumulate in short-term tasks
// NOTE : Please divide the long-term tasks into short-term tasks with different phases
#define TASKSCH_OVERRUN_DETECT_ENABLE       (1)
#define TASKSCH_OVERRUN_DETECT_DISABLE      (0)
#define TASKSCH_OVERRUN_DETECT              (TASKSCH_OVERRUN_DETECT_DISABLE)     // NOTE : USER DEFINE

#define TASKSCH_CYCLE_MEASURE_ENABLE        (1)
#define TASKSCH_CYCLE_MEASURE_DISABLE       (0)
#define TASKSCH_CYCLE_MEASURING             (TASKSCH_CYCLE_MEASURE_DISABLE)      // NOTE : USER DEFINE

#define TASKSCH_INIT_HOOK_ENABLE            (1)
#define TASKSCH_INIT_HOOK_DISABLE           (0)
#define TASKSCH_INIT_HOOK                   (TASKSCH_INIT_HOOK_ENABLE)          // NOTE : USER DEFINE

#define TASKSCH_PRE_EXIT_HOOK_ENABLE        (1)
#define TASKSCH_PRE_EXIT_HOOK_DISABLE       (0)
#define TASKSCH_PRE_EXIT_HOOK               (TASKSCH_PRE_EXIT_HOOK_ENABLE)      // NOTE : USER DEFINE

#define TASKSCH_MAJOR_CYCLE_HOOK_ENABLE     (1)
#define TASKSCH_MAJOR_CYCLE_HOOK_DISABLE    (0)
#define TASKSCH_MAJOR_CYCLE_HOOK            (TASKSCH_MAJOR_CYCLE_HOOK_ENABLE)   // NOTE : USER DEFINE

#define TASKSCH_OVERRUN_HOOK_ENABLE         (1)
#define TASKSCH_OVERRUN_HOOK_DISABLE        (0)
#define TASKSCH_OVERRUN_HOOK                (TASKSCH_OVERRUN_HOOK_ENABLE)       // NOTE : USER DEFINE

#define TASKSCH_WATCHDOG_ENABLE             (1)
#define TASKSCH_WATCHDOG_DISABLE            (0)
#define TASKSCH_TASK_WATCHDOG               (TASKSCH_WATCHDOG_ENABLE)      	    // NOTE : USER DEFINE

/*---- ADDITIONALLY INFORMATION LIST ----*/

#if (TASKSCH_CYCLE_MEASURING == TASKSCH_CYCLE_MEASURE_ENABLE)
    #define TASKSCH_MEASURE_GPIO_PORT               (GPIOA)
    #define TASKSCH_MEASURE_GPIO_PIN                (GPIO_PIN_8)
    #define TASKSCH_START_CYCLE_STAT                (1)
    #define TASKSCH_END_CYCLE_STAT                  (!TASKSCH_START_CYCLE_STAT)
    #define TASKSCH_WRITE_GPIO_PIN(port, pin, stat)  HAL_GPIO_WritePin(port, pin, stat)      
#endif

#if (TASKSCH_TASK_WATCHDOG == TASKSCH_WATCHDOG_ENABLE)
// NOTE : USER DEFINE -> Watchdog clock frequency, prescaler, timeout count
// INFO : LSI frequency can have an error of up to ±20%, so take the count with a margin of error.
    #define TASKSCH_WATCHDOG_CLK_FREQ_HZ    (40000U) 	// Consider LSI at 56khz
    #define TASKSCH_WATCHDOG_PRESCALER      (256U)
    #define TASKSCH_WATCHDOG_PRSCLAER_BITS  (IWDG_PRESCALER_256)
    #define TASKSCH_WATCHDOG_TIMEOUT_CNT    (782)

    // Derived Macros -> Do not modify
    #define TASKSCH_WATCHDOG_1COUNT_MS      (TASKSCH_WATCHDOG_PRESCALER / TASKSCH_WATCHDOG_CLK_FREQ_HZ) // 256 / 40000 = 6.4ms
    #define TASKSCH_WATCHDOG_TIMEOUT_MS     (TASKSCH_WATCHDOG_TIMEOUT_CNT * TASKSCH_WATCHDOG_1COUNT_MS) // 782 * 6.4ms = 5s
#endif

#if (TASKSCH_OVERRUN_DETECT == TASKSCH_OVERRUN_DETECT_ENABLE)
    #define TASKSCH_OVERRUN_THRESHOLD_CNT   (500U)                          // NOTE : USER DEFINE
#endif

/*---- DO NOT MODIFY IT BELOW ----*/

#pragma pack(push, 1) 

typedef struct userRegiTaskObj
{
    void (*regiTaskFunc_ptr)(void);
    uint16_t regiTaskPeriod_ms;
    uint16_t regiTaskOffset_ms;
} typUserRegiTaskObj;

#pragma pack(pop)

#define TASKSCH_ASSERT_FUNC()               do { while(1); } while(0)       // infinite loop 

void tasksch_init_RegiTaskObj(void);
bool tasksch_initWatchdog(void);
bool tasksch_beginWatchdog(void);
bool tasksch_feedWatchdog(void);
bool tasksch_ValidateUser_RegiTaskInfos(void);
void tasksch_getUserRegi_taskInfo(uint8_t taskIdx, typUserRegiTaskObj *taskInfo);
