/**
  ******************************************************************************
  * @file           : mx_spi3.h
  * @brief          : Header for mx_spi3.c file.
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
#ifndef MX_SPI3_H
#define MX_SPI3_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Includes ------------------------------------------------------------------*/
#include "stm32_hal.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/

/** Primary aliases for SPI3_SCK pin */
#define SPI_CLK_PORT                          HAL_GPIOC
#define SPI_CLK_PIN                           HAL_GPIO_PIN_10

/** Primary aliases for SPI3_MISO pin */
#define SPI_MISO_PORT                         HAL_GPIOC
#define SPI_MISO_PIN                          HAL_GPIO_PIN_11

/** Primary aliases for SPI3_MOSI pin */
#define SPI_MOSI_PORT                         HAL_GPIOC
#define SPI_MOSI_PIN                          HAL_GPIO_PIN_12

/** Primary aliases for SPI3_NSS pin */
#define SPI_NSS_PORT                          HAL_GPIOA
#define SPI_NSS_PIN                           HAL_GPIO_PIN_15

/* Exported macros -----------------------------------------------------------*/
/* Exported variables --------------------------------------------------------*/
/* Exported functions ------------------------------------------------------- */
/******************************************************************************/
/* Exported functions for SPI in HAL layer */
/******************************************************************************/
/**
  * @brief mx_spi3 init function
  * This function configures the hardware resources used in this example
  * @retval pointer to handle or NULL in case of failure
  */
hal_spi_handle_t *mx_spi3_init(void);

/**
  * @brief  De-initialize spi3 instance and return it.
  */
void mx_spi3_deinit(void);

/**
  * @brief  Get the SPI3 object.
  * @retval Pointer on the SPI3 Handle
  */
hal_spi_handle_t *mx_spi3_gethandle(void);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* MX_SPI3_H */
