#include "iwdg.h"
#include "stm32f767xx.h"
#include "utils.h"

static typIwdg_handle vIwdg_handler; 

#define IWDG_KEY_ENABLE_ACCESS  0x5555  // PR, RLR 레지스터 쓰기 권한 부여
#define IWDG_KEY_RELOAD         0xAAAA  // 카운터 리로드 (Feed)
#define IWDG_KEY_START          0xCCCC  // 워치독 시작
#define IWDG_CONV_RLR(val)      (val & 0xFFF)

bool iwdg_init(typIwdg_Prescaler prescaler, uint16_t reload_val)
{
    vIwdg_handler.inst = IWDG;

    // CMSIS 레지스터 제어 시작
    vIwdg_handler.inst->KR = IWDG_KEY_START;
    vIwdg_handler.inst->KR = IWDG_KEY_ENABLE_ACCESS; // 쓰기 보호 해제

    if(prescaler > IWDG_DIV_256)
        vIwdg_handler.inst->PR = IWDG_DIV_256;
    else
        vIwdg_handler.inst->PR = prescaler;
    vIwdg_handler.prescaler = prescaler;

    vIwdg_handler.inst->RLR = IWDG_CONV_RLR(reload_val);
    vIwdg_handler.reload_val = reload_val;

    // 레지스터 업데이트 대기 (상태 레지스터 SR의 PVU, RVU 비트가 0이 될 때까지)
    while((vIwdg_handler.inst->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0); // 윈도우비트는 윈도우 자체를 안쓰니까 업데이트 X

    vIwdg_handler.inst->KR = IWDG_KEY_RELOAD; // 세팅한 782 값을 실제 카운터에 밀어 넣기

    vIwdg_handler.isInit = true;
    return true;
}

bool iwdg_start(void)
{
    if(vIwdg_handler.isInit == false) return false;
    
    // 워치독 타이머 가동 -> 이후로는 못 멈춤
    vIwdg_handler.inst->KR = IWDG_KEY_START; 
    return true;
}

inline bool iwdg_feed(void)
{
    vIwdg_handler.inst->KR = IWDG_KEY_RELOAD;
    return true;
}
