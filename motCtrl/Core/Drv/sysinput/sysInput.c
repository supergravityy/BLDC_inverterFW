#include "sysInput.h"
#include "stm32f767xx.h"
#include "utils.h"
#include "main.h"
#include "uart.h"
#include "../mtrCtrl/mtrCtrl.h"
#include "string.h"

#define SYSINPUT_CMD_IDX            (0UL)
#define SYSINPUT_DIV_CH_IDX         (1UL)
#define SYSINPUT_DIV_CH             '>'
#define SYSINPUT_PARA_IDX           (2UL)

typSysInput vSysInput;

inline static void sysInput_clrBuff(void)
{
    vSysInput.rawStr_size = 0;
    memset(vSysInput.rawStr_buff, 0, SYSINPUT_RAW_STR_BUFF_SIZE);
    memset(vSysInput.paraStr_buff, 0, SYSINPUT_RAW_STR_BUFF_SIZE);
}

void sysInput_init(void)
{
    vSysInput.user_turnOff = false;
    vSysInput.userMode = MTRCTRL_CTRL_THROTTLE; // MTRCTRL_CTRL_PI
    vSysInput.userPause = false;
    vSysInput.userKp = SYSINTPUT_OPTI_KP_VAL;
    vSysInput.userKi = SYSINTPUT_OPTI_KI_VAL;

    sysInput_clrBuff();
    uart_debug_enable_rxInterrupt();
}

// TODO : 파싱하는 상위계층 함수 만들기
// TODO : MTR_INVTR_CTRL_MODE에 따라 10MS 태스크 동작 변화시키기 (ifdef)

// 10ms 태스크에서 돌려야 할거같은데 태스크의 딜레이가 발생하지 않을까 걱정

static inline void sysInput_selMode(void)
{
    if (strcmp(vSysInput.paraStr_buff, SYSINPUT_CMD_MODE_THROTTLE) == 0)
    {
        vSysInput.userMode = MTRCTRL_CTRL_THROTTLE;
    }
    else if (strcmp(vSysInput.paraStr_buff, SYSINPUT_CMD_MODE_PI) == 0)
    {
        vSysInput.userMode = MTRCTRL_CTRL_PI;
    }
    else
    {
        // do nothing
    }
}

static inline uint16_t sysInput_strToUint16(const char *string)
{
    uint16_t intVal = 0;

    while (*string)
    {
        if (*string >= '0' && *string <= '9')
            intVal = intVal * 10 + (*string - '0');
        else
            break; // 숫자가 아닌 문자를 만나면 변환 종료

        string++;
    }
    return intVal;
}

static inline float sysInput_strToFloat(const char *string)
{
    float floatVal = 0.0f;
    float fraction = 1.0f;
    bool isFraction = false;

    while (*string)
    {
        if (*string == '.')
        {
            isFraction = true; // 소수점 진입
        }
        else if (*string >= '0' && *string <= '9')
        {
            if (isFraction == false)
            {
                floatVal = floatVal * 10.0f + (float)(*string - '0'); // 정수부 계산: 기존 값 * 10 + 새로운 숫자
            }
            else
            {
                // 소수부 계산: 자리수(0.1, 0.01)를 곱해서 더함
                fraction *= 0.1f;
                floatVal += (float)(*string - '0') * fraction;
            }
        }
        else
        {
            break; // 숫자나 소수점이 아니면 변환 종료
        }

        string++;
    }
    return floatVal;
}

static inline bool sysInput_chkRawStr_isValid(void)
{
    return ((vSysInput.rawStr_size >= SYSINPUT_PARA_IDX) && (vSysInput.rawStr_buff[SYSINPUT_DIV_CH_IDX] == SYSINPUT_DIV_CH));
}

void sysInput_parseData(void)
{
    bool validPara;
    float tempFloat = 0.0f;
    uint16_t tempInt = 0;
    char tempRxBuf[SYSINPUT_RAW_STR_BUFF_SIZE] = {0};
    uint16_t tempRxSize = 0;

    validPara = uart_debug_recvExtract_string(tempRxBuf, &tempRxSize, SYSINPUT_RAW_STR_BUFF_SIZE);

    if (validPara == false || tempRxSize == 0)
        return;

    sysInput_clrBuff();
    strcpy(vSysInput.rawStr_buff, tempRxBuf);
    vSysInput.rawStr_size = tempRxSize;

    if (sysInput_chkRawStr_isValid() == false)
    // 안전 검사: 명령어가 최소 "구분자(>)"를 포함하는 길이(최소 2)인지 확인
        return;

    if (vSysInput.rawStr_size >= 3)
    // 파라미터가 있는 경우(크기가 3 이상)에만 파라미터 버퍼로 복사
    {
        strcpy(vSysInput.paraStr_buff, &vSysInput.rawStr_buff[SYSINPUT_PARA_IDX]);
    }

    switch (vSysInput.rawStr_buff[SYSINPUT_CMD_IDX]) // 명령어 분기
    {
    case SYSINPUT_CMD_SEL_MODE:
        sysInput_selMode();
        break;

    case SYSINPUT_CMD_CHNG_KP:
        tempFloat = sysInput_strToFloat(vSysInput.paraStr_buff);
        if (tempFloat >= 0.0f && tempFloat <= SYSINPUT_KP_MAXVAL)
            vSysInput.userKp = tempFloat;
        break;

    case SYSINPUT_CMD_CHNG_KI:
        tempFloat = sysInput_strToFloat(vSysInput.paraStr_buff);
        if (tempFloat >= 0.0f && tempFloat <= SYSINPUT_KI_MAXVAL)
            vSysInput.userKi = tempFloat;
        break;

    case SYSINPUT_CMD_REF_RPM:
        tempInt = sysInput_strToUint16(vSysInput.paraStr_buff);
        if (tempInt <= SYSINPUT_REF_MAXVAL)
            vSysInput.userRefRPM = tempInt;
        break;

    case SYSINPUT_CMD_CTRL_PAUSE:
        vSysInput.userPause ^= 0x01;
        break;

    case SYSINPUT_CMD_SYSTEM_OFF:
        vSysInput.user_turnOff = true;
        vSysInput.userPause = true;
        break;
    }
    return;
}

/*---- 각 멤버에 맞는 api 작성하기 ----*/

float sysInput_getKp(void)
{
    return vSysInput.userKp;
}
float sysInput_getKi(void)
{
    return vSysInput.userKi;
}
uint16_t sysInput_getRefRPM(void)
{
    return vSysInput.userRefRPM;
}
bool sysInput_getPause(void)
{
    return vSysInput.userPause;
}
bool sysInput_getTurnOff(void)
{
    return vSysInput.user_turnOff;
}
typMtrCtrl_selCtrl_mode sysInput_getMode(void)
{
    return vSysInput.userMode;
}