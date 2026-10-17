/**
  ******************************************************************************
  * @file           : mx_spi3.c
  * @brief          : SPI3 Peripheral initialization
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

/* Includes ------------------------------------------------------------------*/
#include "mx_spi3.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_spi_handle_t hSPI3;

/******************************************************************************/
/* Exported functions for SPI in HAL layer */
/******************************************************************************/
hal_spi_handle_t *mx_spi3_init(void)
{
  hal_spi_config_t spi_config;

  if (HAL_SPI_Init(&hSPI3, HAL_SPI3) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_SPI3_EnableClock();

  if (HAL_RCC_SPI3_SetKernelClkSource(HAL_RCC_SPI3_CLK_SRC_PCLK1) != HAL_OK)
  {
    return NULL;
  }

  spi_config.mode = HAL_SPI_MODE_MASTER;
  spi_config.direction = HAL_SPI_DIRECTION_FULL_DUPLEX;
  spi_config.data_width = HAL_SPI_DATA_WIDTH_8_BIT;
  spi_config.clock_polarity = HAL_SPI_CLOCK_POLARITY_LOW;
  spi_config.clock_phase = HAL_SPI_CLOCK_PHASE_1_EDGE;
  spi_config.baud_rate_prescaler = HAL_SPI_BAUD_RATE_PRESCALER_16;
  spi_config.first_bit = HAL_SPI_MSB_FIRST;
  spi_config.nss_pin_management = HAL_SPI_NSS_PIN_MGMT_OUTPUT;

  if (HAL_SPI_SetConfig(&hSPI3, &spi_config) != HAL_OK)
  {
    return NULL;
  }

  hal_spi_nss_config_t spi_nss_config;

  spi_nss_config.nss_pulse = HAL_SPI_NSS_PULSE_DISABLE;
  spi_nss_config.nss_polarity = HAL_SPI_NSS_POLARITY_LOW;
  spi_nss_config.nss_mssi_delay = HAL_SPI_NSS_MSSI_DELAY_0_CYCLE;

  if (HAL_SPI_SetConfigNSS(&hSPI3, &spi_nss_config) != HAL_OK)
  {
    return NULL;
  }

  /* ### SPI3 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC10    ------>   SPI3_SCK   ------>  SPI_CLK
       PC11    ------>   SPI3_MISO   ------>  SPI_MISO
       PC12    ------>   SPI3_MOSI   ------>  SPI_MOSI
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
  gpio_config.alternate   = HAL_GPIO_AF_6;
  HAL_GPIO_Init(HAL_GPIOC, SPI_CLK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PA15    ------>   SPI3_NSS   ------>  SPI_NSS
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_UP;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_HIGH;
  gpio_config.alternate   = HAL_GPIO_AF_6;
  HAL_GPIO_Init(SPI_NSS_PORT, SPI_NSS_PIN, &gpio_config);

  return &hSPI3;
}

void mx_spi3_deinit(void)
{
  (void)HAL_SPI_DeInit(&hSPI3);

  HAL_RCC_SPI3_Reset();

  HAL_RCC_SPI3_DisableClock();

  /* De-initialize all GPIOC pins associated with SPI3 */
  HAL_GPIO_DeInit(HAL_GPIOC, SPI_CLK_PIN | SPI_MISO_PIN | SPI_MOSI_PIN);

  /* De-initialize all GPIOA pins associated with SPI3 */
  HAL_GPIO_DeInit(SPI_NSS_PORT, SPI_NSS_PIN);
}

hal_spi_handle_t *mx_spi3_gethandle(void)
{
  return &hSPI3;
}
