
/**
  ******************************************************************************
  * @file           : mx_adc2.h
  * @brief          : Header for mx_adc2.c file.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the mx_stm32c5xx_hal_drivers_license.md file
  * in the same directory as the generated code.
  * If no mx_stm32c5xx_hal_drivers_license.md file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef MX_ADC2_H
#define MX_ADC2_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for ADC2_IN4 pin */
#define TH_ADC8_PORT                          HAL_GPIOC
#define TH_ADC8_PIN                           HAL_GPIO_PIN_4

/** Primary aliases for ADC2_IN5 pin */
#define TH_ADC9_PORT                          HAL_GPIOC
#define TH_ADC9_PIN                           HAL_GPIO_PIN_5

/** Primary aliases for ADC2_IN6 pin */
#define TH_ADC10_PORT                         HAL_GPIOB
#define TH_ADC10_PIN                          HAL_GPIO_PIN_0

/** Primary aliases for ADC2_IN7 pin */
#define TH_ADC11_PORT                         HAL_GPIOB
#define TH_ADC11_PIN                          HAL_GPIO_PIN_1

/** Primary aliases for ADC2_IN8 pin */
#define TH_ADC12_PORT                         HAL_GPIOB
#define TH_ADC12_PIN                          HAL_GPIO_PIN_2

/** Primary aliases for ADC2_IN13 pin */
#define TH_ADC4_PORT                          HAL_GPIOH
#define TH_ADC4_PIN                           HAL_GPIO_PIN_5

/** Primary aliases for ADC2_IN0 pin */
#define TH_ADC5_PORT                          HAL_GPIOA
#define TH_ADC5_PIN                           HAL_GPIO_PIN_0

/** Primary aliases for ADC2_IN1 pin */
#define TH_ADC6_PORT                          HAL_GPIOA
#define TH_ADC6_PIN                           HAL_GPIO_PIN_1

/** Primary aliases for ADC2_IN2 pin */
#define TH_ADC7_PORT                          HAL_GPIOA
#define TH_ADC7_PIN                           HAL_GPIO_PIN_2

/** Primary aliases for ADC2_IN9 pin */
#define TH_ADC1_PORT                          HAL_GPIOC
#define TH_ADC1_PIN                           HAL_GPIO_PIN_1

/** Primary aliases for ADC2_IN10 pin */
#define TH_ADC2_PORT                          HAL_GPIOC
#define TH_ADC2_PIN                           HAL_GPIO_PIN_2

/** Primary aliases for ADC2_IN11 pin */
#define TH_ADC3_PORT                          HAL_GPIOC
#define TH_ADC3_PIN                           HAL_GPIO_PIN_3

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for ADC in HAL layer */
/******************************************************************************/
/**
  * @brief mx_adc2 init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_adc_handle_t *mx_adc2_init(void);

/**
  * @brief  De-initialize adc2 instance and return it.
  */
void mx_adc2_deinit(void);

/**
  * @brief  Get the ADC2 object.
  * @retval Pointer on the ADC2 Handle
  */
hal_adc_handle_t *mx_adc2_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_ADC2_H */
