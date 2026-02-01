/*
 * custom_clk.c
 *
 *  Created on: Jan 31, 2026
 *      Author: Smersh
 */

#include "custom_clk.h"
#include "utils.h"

void setUp_clock(void)
{
	// peripheral clock register의 GPIOD 단에 클럭신호 공급
	RCC->AHB1ENR |= SETUP_SHIFT_WRITE_BIT(3,1);
}


void setUp_sysclk(void)
{
	/* 1. 명령 캐시 및 데이터 캐시 설정 */
	SCB_EnableICache();
	SCB_EnableDCache();

	/* 2. ART 가속기*/
	FLASH->ACR = FLASH_ACR_PRFT_EN | FLASH_ACR_ART_EN | 0x07; // flash_cycle[35ns] / 1clk[3.8ns] ~ 7

	/* 3. HSE 및 PLL 설정 */
	RCC->CR |= RCC_CR_HSE_ON | RCC_CR_HSI_ON;
	while((RCC->CR & RCC_CR_HSI_RDY) == 0);

	RCC->CFGR = 0x0; // no clk output + sysclk => hsi
	while((RCC->CFGR & RCC_CFGR_NOT_RDY) != 0x0);

	//PLL 설정 순서
	// 1) 현재 SYSCLK 소스를 HSI로 변경하여 안전한 기본 상태 확보
	// 2) PLL을 끄고 PLL 설정값 초기화 (PLLM, PLLN, PLLP 등)
	// 3) PLL 파라미터 설정 (HSE → PLL 입력, 내부 VCO 구성)
	// 4) PLL을 다시 켜고 안정될 때까지 대기 (PLLRDY = 1)
	// 5) SYSCLK 소스를 PLL로 변경
	RCC->CR = RCC_CR_HSE_ON | RCC_CR_HSI_ON;
	RCC->PLLCFGR = 0x09403608;  //1001 0100 0000 0011 0110 0000 1000
	    						// HSE 16MHz를 8로 나눠서 2MHz를 만듬 --> PLL 내부의 VCO회로가 1~2MHz에서 안정적
	    						// 2MHz에 216을 곱해서 432MHz --> 다시 2로 나눠서 216MHz 클럭을 생성
	    						// SYSCLK = HSE*PLLN/PLLM/PLLP = 16MHz*216/8/2 = 216MHz
	                            // PLL48CK = HSE*PLLN/PLLM/PLLQ = 16MHz*216/8/9 = 48MHz
	RCC->CR = RCC_CR_PLL_ON | RCC_CR_HSE_ON | RCC_CR_HSI_ON;
	while((RCC->CR & RCC_CR_PLL_RDY) == 0);

	/* 4. 오버드라이브 설정 */
	// 오버드라이브를 사용하지 않으면 STM32F767의 최대 속도는 180MHz -> 전압레귤레이터 출력 ↑ ->216mhz
	RCC->APB1ENR |= RCC_APB1ENR_PWR; // 전원모듈 clk
	PWR->CR1 |= PWR_CR1_OD_EN;
	while((PWR->CSR1 & PWR_CSR1_OD_RDY) == 0);
	PWR->CR1 |= PWR_CR1_ODSW_EN;
	while((PWR->CSR1 & PWR_CSR1_ODSW_RDY) == 0);
	//코어 216MHz 설정 완료

	/* 5. 주변장치 클록 설정 */
	RCC->CFGR = 0x3040B402; // SYSCLK = PLL, AHB = 216MHz, APB1 = APB2 = 54MHz
	RCC->DCKCFGR1 = RCC_DCKCFGR1_TIMP;
	while((RCC->CFGR & RCC_CFGR_NOT_RDY) != 0x00000008); // wait until SYSCLK = PLL
	RCC->CR |= RCC_CR_CSS_ON; // CLK 불안정 디텍션모드 on

	/* 6. IO 보상 설정 */
	// 보상 셀은 GPIO 출력 드라이버의 임피던스를 제어 -> 내부적으로 디지털 제어된 커패시터 및 버퍼로 Slew rate 보정 수행
	RCC->APB2ENR |= RCC_APB2ENR_SYSCFG; // 주변장치 클록(SYSCFG = 1)
	SYSCFG->CMPCR = 0x00000001; // enable compensation cell
}
