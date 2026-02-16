#include "stm32f767xx.h"
#include "utils.h"
#include "system.h"
#include "gpio.h"
#include "tim.h"
#include "adc.h"
#include "dac.h"
#include "isr.h"
#include "uart.h"
#include "../Drv/tasksch/tasksch.h"
#include "../Drv/mtrCtrl/mtrCtrl.h"

int main(void)
{
    system_clock_init();
    system_sysTick_init();
    gpio_init();
    tim_init();
    exti_init();
    adc_init();
    dac_init();
    uart_AT09_init(9600, 8, 1, UART_PARITY_NONE);
    uart_debug_init(9600, 8, 1, UART_PARITY_NONE);
    tasksch_init();
    mtrCtrl_objInit(0,0);
    mtrCtrl_setPeriphInit();

    tim_pwm1_nvic_counterSet(); // TIM1의 ISR에서 모듈을 사용하기에 제일 마지막에 호출 될 것

    tasksch_execTask();
}