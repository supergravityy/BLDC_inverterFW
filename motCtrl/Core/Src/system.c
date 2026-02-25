#include "system.h"

static inline void system_clock_pll_init(void)
{
    //PLL 설정 순서
	// 1) 현재 SYSCLK 소스를 HSI로 변경하여 안전한 기본 상태 확보
	// 2) PLL을 끄고 PLL 설정값 초기화 (PLLM, PLLN, PLLP 등)
	// 3) PLL 파라미터 설정 (HSE → PLL 입력, 내부 VCO 구성)
	// 4) PLL을 다시 켜고 안정될 때까지 대기 (PLLRDY = 1)
	// 5) SYSCLK 소스를 PLL로 변경

    RCC->CR |= RCC_CR_HSEON | RCC_CR_HSION;
    while(!(RCC->CR & RCC_CR_HSIRDY));    // HSI 안정화 대기

    RCC->CFGR &= ~RCC_CFGR_SW;            // 시스템 클록을 일단 HSI로 설정
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

    RCC->CR |= RCC_CR_HSEON | RCC_CR_HSION; // PLL OFF 상태 유지
    RCC->PLLCFGR = (9 << RCC_PLLCFGR_PLLQ_Pos) | RCC_PLLCFGR_PLLSRC_HSE | 
                   (0 << RCC_PLLCFGR_PLLP_Pos) | (216 << RCC_PLLCFGR_PLLN_Pos) | 8;

    RCC->CR = RCC_CR_PLLON | RCC_CR_HSEON | RCC_CR_HSION; // pll on
    while(!(RCC->CR & RCC_CR_PLLRDY));
}

static inline void system_clock_pll_init2(void)
{
    // 최종 클록 분주비 및 PLL 소스 전환
    RCC->CFGR = RCC_CFGR_PPRE2_DIV4 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_HPRE_DIV1 | RCC_CFGR_SW_PLL;

    // 타이머 클록 프리프레임 설정
    RCC->DCKCFGR1 = RCC_DCKCFGR1_TIMPRE;

    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL); // 클럭소스 PLL 전환 대기
}

static inline void system_clock_periperals(void)
{
    /* 1. AHB1 Bus: GPIOA~E (모든 IO 포트 일괄 활성화) */
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | 
        RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN);

    /* 2. APB1 Bus: TIM2, DAC, USART2 */
    RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN | RCC_APB1ENR_DACEN | RCC_APB1ENR_USART2EN);

    /* 3. APB2 Bus: TIM1, ADC1~3, SYSCFG */
    RCC->APB2ENR |= (RCC_APB2ENR_TIM1EN | RCC_APB2ENR_ADC1EN | RCC_APB2ENR_ADC2EN | 
        RCC_APB2ENR_ADC3EN | RCC_APB2ENR_SYSCFGEN);
}

void system_mcu_init(void)
{
    /* 1. L1 캐시 활성화 (고속 실행) */
    SCB_EnableICache();
    SCB_EnableDCache();

    /* 2. Flash ACR 설정: 7 Wait States 적용 및 ART 가속기 활성화 */
    // FLASH_ACR_LATENCY_7WS (0x7), FLASH_ACR_ARTEN, FLASH_ACR_PRFTEN
    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN;

    /* 3. 시스템 클록 설정 (HSE 기반 216MHz) */
    RCC->CR |= RCC_CR_HSEON | RCC_CR_HSION;
    while(!(RCC->CR & RCC_CR_HSIRDY));    // HSI 안정화 대기

    RCC->CFGR &= ~RCC_CFGR_SW;            // 시스템 클록을 일단 HSI로 설정
    while((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI);

    /* 4. HSE 설정 및 PLL 설정 */
    system_clock_pll_init();

    /* 5. 오버드라이브 설정 -> 216MHz 진입 전 전압 강화 */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR1 |= PWR_CR1_ODEN;
    while(!(PWR->CSR1 & PWR_CSR1_ODRDY));
    
    PWR->CR1 |= PWR_CR1_ODSWEN;
    while(!(PWR->CSR1 & PWR_CSR1_ODSWRDY));

    /* 6. 최종 클록 분주비 및 PLL로 클럭소스 전환 */
    system_clock_pll_init2();

    /* 7. CSS 활성화 */
    RCC->CR |= RCC_CR_CSSON;

    /* 8. I/O 보상 및 주변장치 클록 활성화 */
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    SYSCFG->CMPCR = SYSCFG_CMPCR_CMP_PD;

    system_clock_periperals();
}

void system_sysTick_init(void)
{
    // SYSCLK가 216MHz일 때, 1ms마다 인터럽트가 발생하도록 설정:
    // Reload = (216,000,000 / 1000) - 1 = 215999
    SysTick->LOAD = (216000000 / 1000) - 1;
    SysTick->VAL  = 0;  // 현재 카운터 값 초기화
    // SysTick 제어: 프로세서 클록(216MHz) 사용, 인터럽트 활성, 카운터 시작
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_TICKINT_Msk | SysTick_CTRL_ENABLE_Msk;
}