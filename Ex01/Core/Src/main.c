#include "stm32f767xx.h"
#include "utils.h"
#include "custom_clk.h"
#include "custom_gpio.h"
#include "custom_exti.h"
#include "custom_adc.h"

int main(void)
{
	// 1. clock을 제일먼저 설정해줘야 함

	//setUp_clock();
	setUp_sysclk();

	// 2. GPIO 설정
	setUp_GPIO();
	setUp_exti();
	setUp_adc();

	// 3. 컨트롤
	while(1)
	{	control_delay(20000000);
		if(control_GPIO_read(GPIOD, GPIOD_SWT_BTTN) == 0)
		{
			control_GPIO_write(GPIOD, GPIOD_FLT_LED, 1);
		}
		else
		{
			control_GPIO_write(GPIOD, GPIOD_FLT_LED, 0);
		}
	}
}
