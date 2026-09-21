
/**
  ******************************************************************************
  * @file           : mx_adc1.h
  * @brief          : Header for mx_adc1.c file.
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
#ifndef MX_ADC1_H
#define MX_ADC1_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for ADC1_IN5 pin */
#define DUT_ADC_PORT                          HAL_GPIOA
#define DUT_ADC_PIN                           HAL_GPIO_PIN_5

/** Primary aliases for ADC1_IN6 pin */
#define I_ADC_SIDE21_PORT                     HAL_GPIOA
#define I_ADC_SIDE21_PIN                      HAL_GPIO_PIN_6

/** Primary aliases for ADC1_IN7 pin */
#define I_ADC_SIDE11_PORT                     HAL_GPIOA
#define I_ADC_SIDE11_PIN                      HAL_GPIO_PIN_7

/** Primary aliases for ADC1_IN8 pin */
#define I_ADC_SIDE22_PORT                     HAL_GPIOC
#define I_ADC_SIDE22_PIN                      HAL_GPIO_PIN_0

/** Primary aliases for ADC1_IN3 pin */
#define I_ADC_SIDE12_PORT                     HAL_GPIOA
#define I_ADC_SIDE12_PIN                      HAL_GPIO_PIN_3

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for ADC in HAL layer */
/******************************************************************************/
/**
  * @brief mx_adc1 init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_adc_handle_t *mx_adc1_init(void);

/**
  * @brief  De-initialize adc1 instance and return it.
  */
void mx_adc1_deinit(void);

/**
  * @brief  Get the ADC1 object.
  * @retval Pointer on the ADC1 Handle
  */
hal_adc_handle_t *mx_adc1_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_ADC1_H */
