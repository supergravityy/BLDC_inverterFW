#pragma once

#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "uart.h"
#include "../mtrCtrl/mtrCtrl.h"

#define SYSINPUT_RAW_STR_BUFF_SIZE      (20UL)

#define SYSINPUT_CMD_SEL_MODE           'm'
#define SYSINPUT_CMD_CHNG_KP            'p'
#define SYSINPUT_CMD_CHNG_KI            'i'
#define SYSINPUT_CMD_REF_RPM            'r'
#define SYSINPUT_CMD_CTRL_PAUSE         't'
#define SYSINPUT_CMD_SYSTEM_OFF         'q'

#define SYSINPUT_CMD_MODE_THROTTLE      "th"
#define SYSINPUT_CMD_MODE_PI            "pi"
#define SYSINPUT_KP_MAXVAL              (5.f)
#define SYSINPUT_KI_MAXVAL              (10.f)
#define SYSINPUT_REF_MAXVAL             (4000UL)

#define SYSINTPUT_OPTI_KP_VAL           (3.4f)
#define SYSINTPUT_OPTI_KI_VAL           (5.32f)

typedef struct sysInput
{
    float userKp;
    float userKi;
    uint16_t userRefRPM;
    bool userPause;
    bool user_turnOff;
    typMtrCtrl_selCtrl_mode userMode;

    uint16_t rawStr_size;
    char rawStr_buff[SYSINPUT_RAW_STR_BUFF_SIZE];
    char paraStr_buff[SYSINPUT_RAW_STR_BUFF_SIZE];
}typSysInput;

void sysInput_init(void);
void sysInput_parseData(void);

float sysInput_getKp(void);
float sysInput_getKi(void);
uint16_t sysInput_getRefRPM(void);
bool sysInput_getPause(void);
bool sysInput_getTurnOff(void);
typMtrCtrl_selCtrl_mode sysInput_getMode(void);
