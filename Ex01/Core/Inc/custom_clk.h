/*
 * custom_clk.h
 *
 *  Created on: Jan 31, 2026
 *      Author: Smersh
 */

#ifndef INC_CUSTOM_CLK_H_
#define INC_CUSTOM_CLK_H_

#include "stm32f767xx.h"

#define FLASH_ACR_ART_EN	(SETUP_SHIFT_WRITE_BIT(9,1)) // 고속 실행 가속기 (ART) 활성화
#define FLASH_ACR_PRFT_EN	(SETUP_SHIFT_WRITE_BIT(8,1)) // 플래시에서 명령어를 미리 읽어두기 (prefetch)

#define RCC_CR_HSE_ON		(SETUP_SHIFT_WRITE_BIT(16,1))
#define RCC_CR_HSI_RDY		(SETUP_SHIFT_WRITE_BIT(1,1))
#define RCC_CR_HSI_ON		(1U)
#define RCC_CR_CSS_ON		(SETUP_SHIFT_WRITE_BIT(19,1))
#define RCC_CR_PLL_ON		(SETUP_SHIFT_WRITE_BIT(24,1))
#define RCC_CR_PLL_RDY		(SETUP_SHIFT_WRITE_BIT(25,1))

#define RCC_CFGR_NOT_RDY	(0x000C)

#define PWR_CR1_OD_EN		(SETUP_SHIFT_WRITE_BIT(16,1))
#define PWR_CR1_ODSW_EN		(SETUP_SHIFT_WRITE_BIT(17,1))
#define PWR_CSR1_OD_RDY		(SETUP_SHIFT_WRITE_BIT(16,1))
#define PWR_CSR1_ODSW_RDY	(SETUP_SHIFT_WRITE_BIT(17,1))

#define RCC_APB1ENR_PWR		(SETUP_SHIFT_WRITE_BIT(28,1))
#define RCC_APB2ENR_SYSCFG	(SETUP_SHIFT_WRITE_BIT(14,1))
#define RCC_DCKCFGR1_TIMP	(SETUP_SHIFT_WRITE_BIT(24,1))

void setUp_sysclk(void);
void setUp_clock(void);

#endif /* INC_CUSTOM_CLK_H_ */
