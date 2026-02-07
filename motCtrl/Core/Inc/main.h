/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f7xx_hal.h"

/* --- ADC --- */
#define IAS_ADC_Port          GPIOA
#define IAS_ADC_Pin           GPIO_PIN_0
#define IBS_ADC_Port          GPIOA
#define IBS_ADC_Pin           GPIO_PIN_1
#define ICS_ADC_Port          GPIOA
#define ICS_ADC_Pin           GPIO_PIN_2
#define VDC_ADC_Port          GPIOA
#define VDC_ADC_Pin           GPIO_PIN_3
#define MOSFET_NTC_Port       GPIOA
#define MOSFET_NTC_Pin        GPIO_PIN_6
#define THROTTLE_Port         GPIOA
#define THROTTLE_Pin          GPIO_PIN_7

/* --- DAC --- */
#define DAC_OUT1_Port         GPIOA
#define DAC_OUT1_Pin          GPIO_PIN_4
#define DAC_OUT2_Port         GPIOA
#define DAC_OUT2_Pin          GPIO_PIN_5

/* --- JTAG --- */
#define JTAG_TRST_Port        GPIOB   
#define JTAG_TRST_Pin         GPIO_PIN_4
#define JTAG_TDI_Port         GPIOA
#define JTAG_TDI_Pin          GPIO_PIN_15
#define JTAG_TMS_Port         GPIOA
#define JTAG_TMS_Pin          GPIO_PIN_13
#define JTAG_TCK_Port         GPIOA
#define JTAG_TCK_Pin          GPIO_PIN_14
#define JTAG_TDO_Port         GPIOB
#define JTAG_TDO_Pin          GPIO_PIN_3

/* --- GPIO --- */
#define BRK_Port              GPIOB
#define BRK_Pin               GPIO_PIN_15
#define FLT_LED_Port          GPIOC
#define FLT_LED_Pin           GPIO_PIN_6

/* --- EXTI --- */
#define HALL_U_Port           GPIOD
#define HALL_U_Pin            GPIO_PIN_0
#define HALL_V_Port           GPIOD
#define HALL_V_Pin            GPIO_PIN_1
#define HALL_W_Port           GPIOD
#define HALL_W_Pin            GPIO_PIN_2
#define SWITCH_Port           GPIOD
#define SWITCH_Pin            GPIO_PIN_4
#define STATE_Port            GPIOD
#define STATE_Pin             GPIO_PIN_10

/* --- PWM --- */
#define PWM_U_BOT_Port        GPIOE
#define PWM_U_BOT_Pin         GPIO_PIN_8
#define PWM_U_TOP_Port        GPIOE
#define PWM_U_TOP_Pin         GPIO_PIN_9
#define PWM_V_BOT_Port        GPIOE
#define PWM_V_BOT_Pin         GPIO_PIN_10
#define PWM_V_TOP_Port        GPIOE
#define PWM_V_TOP_Pin         GPIO_PIN_11
#define PWM_W_BOT_Port        GPIOE
#define PWM_W_BOT_Pin         GPIO_PIN_12
#define PWM_W_TOP_Port        GPIOE
#define PWM_W_TOP_Pin         GPIO_PIN_13

/* --- I2C --- */
#define I2C4_SCL_Port         GPIOD
#define I2C4_SCL_Pin          GPIO_PIN_12
#define I2C4_SDA_Port         GPIOD
#define I2C4_SDA_Pin          GPIO_PIN_13

/* --- UART --- */
#define USART2_TX_Port        GPIOD
#define USART2_TX_Pin         GPIO_PIN_5
#define USART2_RX_Port        GPIOD
#define USART2_RX_Pin         GPIO_PIN_6
#define BLUETOOTH_TX_Port     GPIOD
#define BLUETOOTH_TX_Pin      GPIO_PIN_8
#define BLUETOOTH_RX_Port     GPIOD
#define BLUETOOTH_RX_Pin      GPIO_PIN_9

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
