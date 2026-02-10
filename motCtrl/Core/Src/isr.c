#include "isr.h"
#include "utils.h"
#include "tim.h"
#include "../Drv/tasksch/tasksch.h"

void SysTick_Handler(void)
{
    tasksch_timeManager();
}

// 원래 EXTI0~4 까지는 단 하나의 핀하고 연결(SYSCFG->EXTICR)되서 PR레지스터만 clr해주면 됨
// 하지만, 전기적인 노이즈 때문에 PR을 한번 더 확인함 방어적 프로그래밍임
void EXTI0_IRQHandler(void)
{
    if (exti_check_extiPR(0)) 
    {
        exti_clear_extiPR(0); // 인터럽트 플래그 클리어
        //Update_Hall_Sequence();
        //SpeedCal();
    }
}

void EXTI1_IRQHandler(void)
{
    if (exti_check_extiPR(1))   
    {
        exti_clear_extiPR(1); // 인터럽트 플래그 클리어
        //Update_Hall_Sequence();
        //SpeedCal();
    }
}

void EXTI2_IRQHandler(void)
{
	if (exti_check_extiPR(2))
	{
        exti_clear_extiPR(2); // 인터럽트 플래그 클리어
        //Update_Hall_Sequence();
        //SpeedCal();
    }
}

void TIM1_UP_TIM10_IRQHandler(void)
{
    
}

/* void TIM1_UP_TIM10_IRQHandler(void)
{
    // [1] 모니터링용 카운터 및 속도 계산
    Tim1TestCnt++; // ISR 실행 횟수 체크용

    RpmNew = calculated_rpm;

    // 속도가 변하지 않을 때 (정지 상태 판별)
    if(RpmNew == RpmOld)
    {
        rpmHoldCounter++;
        if(rpmHoldCounter > 20000) // 약 1초 동안 속도 변화 없으면 (20kHz 기준)
        {
            calculated_rpm = 0.0f;
            rpmHoldCounter = 0;
        }
    }
    RpmOld = calculated_rpm;

    // [2] 메인 제어 루프 진입 (Update Interrupt Flag 및 초기화 완료 확인) 
    // (TIM1->SR & 0x0001) -> (TIM1->SR & TIM_SR_UIF)
    if ((TIM1->SR & TIM_SR_UIF) && InitCal == 1) 
    {
        uint32_t result = 0;

        // 인터럽트 플래그 클리어
        TIM1->SR &= ~TIM_SR_UIF;

        // [3] 상전류 및 쓰로틀 아날로그 데이터 수집 (Polling 방식) 
        // PA0 (ias) - ADC1
        ADC1->SQR3 = 0x00000000;    // SQ1=0 (채널 0 선택)
        ADC1->CR2 |= 0x40000000;    // (ADC_CR2_SWSTART) 소프트웨어 트리거 시작
        while(!(ADC1->SR & 0x00000002)); // (ADC_SR_EOC) 변환 완료 대기
        result = ADC1->DR;          // 데이터 읽기
        // 전류 변환식: (Raw값 - Offset) * 전압계수 / 게인
        ias_Cal = ((float)(result - Ias_Offset) * ADC_VREF / ADC_FS - OFFSET_Volt) / OPAMP_GAIN;
        ias = ias_Cal;
        LPF(ias, Fi, &ias_LPF);     // 저역 통과 필터링

        // PA1 (ibs) - ADC2
        ADC2->SQR3 = 0x00000001;    // SQ1=1 (채널 1 선택)
        ADC2->CR2 |= 0x40000000;    // (ADC_CR2_SWSTART)
        while(!(ADC2->SR & 0x00000002)); // (ADC_SR_EOC)
        result = ADC2->DR;
        ibs_Cal = ((float)(result - Ibs_Offset) * ADC_VREF / ADC_FS - OFFSET_Volt) / OPAMP_GAIN;
        ibs = ibs_Cal;
        LPF(ibs, Fi, &ibs_LPF);

        // PA2 (ics) - ADC3
        ADC3->SQR3 = 0x00000002;    // SQ1=2 (채널 2 선택)
        ADC3->CR2 |= 0x40000000;    // (ADC_CR2_SWSTART)
        while(!(ADC3->SR & 0x00000002)); // (ADC_SR_EOC)
        result = ADC3->DR;
        ics_Cal = ((float)(result - Ics_Offset) * ADC_VREF / ADC_FS - OFFSET_Volt) / OPAMP_GAIN;
        ics = ics_Cal;
        LPF(ics, Fi, &ics_LPF);

        // PA7 (Throttle) - ADC2
        ADC2->SQR3 = 0x00000007;    // SQ1=7 (채널 7 선택)
        ADC2->CR2 |= 0x40000000;    // (ADC_CR2_SWSTART)
        while(!(ADC2->SR & 0x00000002)); // (ADC_SR_EOC)
        result = ADC2->DR;
        Throttle_ADC = (float)result * 3.3f / 4095.0f;

        // [4] 보호 로직 및 이상 감지 (Safety) 
        I_Max = MAX3(ias, ibs, ics); // 3상 중 최대 전류 추출

        if(I_Max > OC_LEVEL) // 과전류 감지 시
        {
            FltCnt++;
            if(FltCnt >= 1000) // 50ms (20kHz * 1000회) 지속 시 트립
            {
                FltFlg = 1;
                ThrottleRef = 0;
            }
        }
        else if(I_Max < 30.0f) // 정상 범위면 카운터 초기화
        {
            FltCnt = 0;
        }

        // [5] 사용자 명령 처리 및 속도 변환 
        LPF(calculated_rpm, Ft, &motor_speed_rpm);
        speed_km_h = RPM_TO_KMH(motor_speed_rpm);

        // 쓰로틀 히스테리시스 (미세한 떨림 방지)
        if (Throttle_ADC < THROTTLE_OFF) ThrottleActive = 0;
        else if (Throttle_ADC > THROTTLE_ON) ThrottleActive = 1;

        if (!ThrottleActive) // 정지 모드
        {
            Disable_PWM(); // (Mute All)
            ThrottleRef = 0.0f;
            VoltageRef = 0;
        }
        else // 구동 모드
        {
            // 전압 지령치 매핑 (0~3.3V 입력 -> 듀티값 범위로 변환)
            ThrottleRef = Throttle_ADC * 3400.0f - 3540.0f + 1000.0f; 
        }

        // 가속 램프 제어 (급발진 방지)
        rampToTarget(ThrottleRef, &ThrottleRef_Ramp, iRamp);

        if(SpdFlg == 0)
        {
            VoltageRef = (uint32_t)ThrottleRef_Ramp;
            if(VoltageRef > CNT_MAX - 100) VoltageRef = CNT_MAX - 100; // 최대 출력 제한(마진)
        }

        // [6] 최종 PWM 출력 업데이트 (6-Step Commutation) 
        if(FltFlg == 0 && InitCal == 1) // 에러 없고 초기화 완료 시
        {
            Update_Switching_Pattern(HallSum); // 홀 센서 기반 스위칭 (Set_Phases 호출)
        }
        else // 에러 상황 시 완전 차단
        {
            VoltageRef = 0;
            DutyA = DutyB = DutyC = 0;
            TIM1->CCR1 = TIM1->CCR2 = TIM1->CCR3 = 0; // 하드웨어 듀티 0
            // 여기에 Disable_PWM() 또는 Mute_All 추가 필요
        }
    }
} */