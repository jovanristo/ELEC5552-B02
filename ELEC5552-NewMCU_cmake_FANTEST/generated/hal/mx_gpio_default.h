/**
  ******************************************************************************
  * @file           : mx_gpio_default.h
  * @brief          : Header for mx_gpio_default.c file.
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
#ifndef MX_GPIO_DEFAULT_H
#define MX_GPIO_DEFAULT_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"
#include "mx_def.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/******************************************************************************/
/* Exported defines for gpio_default in HAL layer                             */
/******************************************************************************/

/* Primary aliases for GPIO PA9 pin */
#define USB_VBUS_PORT                                   HAL_GPIOA
#define USB_VBUS_PIN                                    HAL_GPIO_PIN_9

/* Primary aliases for GPIO PB3(JTDO/TRACESWO) pin */
#define BUTTON_DOWN_PORT                                HAL_GPIOB
#define BUTTON_DOWN_PIN                                 HAL_GPIO_PIN_3

/* Primary aliases for GPIO PB4(NJTRST) pin */
#define BUTTON_OK_PORT                                  HAL_GPIOB
#define BUTTON_OK_PIN                                   HAL_GPIO_PIN_4

/* Primary aliases for GPIO PB5 pin */
#define BUTTON_0_PORT                                   HAL_GPIOB
#define BUTTON_0_PIN                                    HAL_GPIO_PIN_5

/* Primary aliases for GPIO PB6 pin */
#define BUTTON_1_PORT                                   HAL_GPIOB
#define BUTTON_1_PIN                                    HAL_GPIO_PIN_6

/* Primary aliases for GPIO PB7 pin */
#define BUTTON_2_PORT                                   HAL_GPIOB
#define BUTTON_2_PIN                                    HAL_GPIO_PIN_7

/* Primary aliases for GPIO PB8 pin */
#define BUTTON_3_PORT                                   HAL_GPIOB
#define BUTTON_3_PIN                                    HAL_GPIO_PIN_8

/* Primary aliases for GPIO PB9 pin */
#define BUTTON_4_PORT                                   HAL_GPIOB
#define BUTTON_4_PIN                                    HAL_GPIO_PIN_9

/* Primary aliases for GPIO PB12 pin */
#define SW_SEL_PORT                                     HAL_GPIOB
#define SW_SEL_PIN                                      HAL_GPIO_PIN_12

/* Primary aliases for GPIO PB13 pin */
#define MODE_SEL_PORT                                   HAL_GPIOB
#define MODE_SEL_PIN                                    HAL_GPIO_PIN_13

/* Primary aliases for GPIO PC13 pin */
#define BUTTON_9_PORT                                   HAL_GPIOC
#define BUTTON_9_PIN                                    HAL_GPIO_PIN_13

/* Primary aliases for GPIO PD0 pin */
#define BUTTON_LEFT_PORT                                HAL_GPIOD
#define BUTTON_LEFT_PIN                                 HAL_GPIO_PIN_0

/* Primary aliases for GPIO PD1 pin */
#define BUTTON_UP_PORT                                  HAL_GPIOD
#define BUTTON_UP_PIN                                   HAL_GPIO_PIN_1

/* Primary aliases for GPIO PD2 pin */
#define BUTTON_RIGHT_PORT                               HAL_GPIOD
#define BUTTON_RIGHT_PIN                                HAL_GPIO_PIN_2

/* Primary aliases for GPIO PE0 pin */
#define BUTTON_5_PORT                                   HAL_GPIOE
#define BUTTON_5_PIN                                    HAL_GPIO_PIN_0

/* Primary aliases for GPIO PE1 pin */
#define BUTTON_6_PORT                                   HAL_GPIOE
#define BUTTON_6_PIN                                    HAL_GPIO_PIN_1

/* Primary aliases for GPIO PE2 pin */
#define BUTTON_7_PORT                                   HAL_GPIOE
#define BUTTON_7_PIN                                    HAL_GPIO_PIN_2

/* Primary aliases for GPIO PE3 pin */
#define BUTTON_8_PORT                                   HAL_GPIOE
#define BUTTON_8_PIN                                    HAL_GPIO_PIN_3

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for gpio_default in HAL layer                           */
/******************************************************************************/
/**
  * @brief mx_gpio_default init function
  * This function configures the hardware resources used in this example
  * @retval 0  GPIO group correctly initialized
  * @retval -1 Issue detected during GPIO group initialization
  */
system_status_t mx_gpio_default_init(void);

/**
  * @brief  De-initialize gpio_default instance.
  */
system_status_t mx_gpio_default_deinit(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_GPIO_DEFAULT_H */
