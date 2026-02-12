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
#include "../Drv/hallSens/hallsens.h"
#include "../Drv/mtrCtrl/mtrCtrl.h"

typMotorCtrl_handle vMotorCtrl_handler;

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

    utils_LPF_temper_init();
    utils_LPF_phaseCurr_init();

    adc_offsetCalib_curr(vMotorCtrl_handler.phaseCurr_offset);

    if(vMotorCtrl_handler.errCode == MTRCTRL_ERR_ADC_FAIL)
    {
        vMotorCtrl_handler.calib_cmplt = false;
    }
    else
    {
        vMotorCtrl_handler.calib_cmplt = true;
    }

    tim_pwm1_nvic_counterSet();
    hallsens_init();
}