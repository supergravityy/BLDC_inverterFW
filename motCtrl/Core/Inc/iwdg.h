#pragma once 

#include "stm32f767xx.h"
#include "utils.h"

typedef enum Iwdg_Prescaler
{
    IWDG_DIV_4   = 0x00,
    IWDG_DIV_8   = 0x01,
    IWDG_DIV_16  = 0x02,
    IWDG_DIV_32  = 0x03,
    IWDG_DIV_64  = 0x04,
    IWDG_DIV_128 = 0x05,
    IWDG_DIV_256 = 0x06
} typIwdg_Prescaler;

typedef struct Iwdg_handle
{
    IWDG_TypeDef* inst;    // CMSIS IWDG 레지스터 맵 포인터
    typIwdg_Prescaler prescaler;
    uint16_t reload_val;        // 0 ~ 4095
    bool          isInit;       // 초기화 완료 플래그
} typIwdg_handle;

bool iwdg_init(typIwdg_Prescaler prescaler, uint16_t reload_val);
bool iwdg_start(void);
bool iwdg_feed(void);