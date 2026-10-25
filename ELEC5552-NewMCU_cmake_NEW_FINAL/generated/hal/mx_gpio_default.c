/**
  ******************************************************************************
  * @file           : mx_gpio_default.c
  * @brief          : gpio_default Peripheral initialization
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
#include "mx_gpio_default.h"

/* Exported types ------------------------------------------------------------*/
/* Exported constants --------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* Exported variables by reference -------------------------------------------*/

/******************************************************************************/
/* Exported functions for GPIO in HAL layer                                   */
/******************************************************************************/
system_status_t mx_gpio_default_init(void)
{
  hal_gpio_config_t  gpio_config;

  HAL_RCC_GPIOA_EnableClock();

  HAL_RCC_GPIOB_EnableClock();

  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOD_EnableClock();

  HAL_RCC_GPIOE_EnableClock();

  /*
    GPIO pin labels :
    PA9   ---------> USB_VBUS
    */
  /* Configure PA9 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(USB_VBUS_PORT, USB_VBUS_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB3   ---------> BUTTON_DOWN
    PB4   ---------> BUTTON_OK
    PB5   ---------> BUTTON_0
    PB6   ---------> BUTTON_1
    PB7   ---------> BUTTON_2
    PB8   ---------> BUTTON_3
    PB9   ---------> BUTTON_4
    */
  /* Configure PB3, PB4, PB5, PB6, PB7, PB8, PB9 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(HAL_GPIOB, BUTTON_DOWN_PIN | BUTTON_OK_PIN | BUTTON_0_PIN | BUTTON_1_PIN | BUTTON_2_PIN | BUTTON_3_PIN | BUTTON_4_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PB12  ---------> SW_SEL
    PB13  ---------> MODE_SEL
    */
  /* Configure PB12, PB13 GPIO pins in output mode */
  /* MOSFET clamp and CC/CV selection */
  gpio_config.mode = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull = HAL_GPIO_PULL_NO;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state = HAL_GPIO_PIN_SET;

  if (HAL_GPIO_Init(HAL_GPIOB,
      SW_SEL_PIN | MODE_SEL_PIN,
      &gpio_config) != HAL_OK)
  {
      return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC6   ---------> LCD_DC
    */
  /* Configure PC6 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = LCD_DC_INIT_STATE;
  if (HAL_GPIO_Init(LCD_DC_PORT, LCD_DC_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC7   ---------> LCD_RST
    */
  /* Configure PC7 GPIO pin in output mode */
  gpio_config.mode            = HAL_GPIO_MODE_OUTPUT;
  gpio_config.speed           = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  gpio_config.output_type     = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.init_state      = LCD_RST_INIT_STATE;
  if (HAL_GPIO_Init(LCD_RST_PORT, LCD_RST_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /* Configure PC8, PC9 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_NO;
  if (HAL_GPIO_Init(HAL_GPIOC, HAL_GPIO_PIN_8 | HAL_GPIO_PIN_9, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PC13  ---------> BUTTON_9
    */
  /* Configure PC13 GPIO pin in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(BUTTON_9_PORT, BUTTON_9_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PD0   ---------> BUTTON_LEFT
    PD1   ---------> BUTTON_UP
    PD2   ---------> BUTTON_RIGHT
    */
  /* Configure PD0, PD1, PD2 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(HAL_GPIOD, BUTTON_LEFT_PIN | BUTTON_UP_PIN | BUTTON_RIGHT_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  /*
    GPIO pin labels :
    PE0   ---------> BUTTON_5
    PE1   ---------> BUTTON_6
    PE2   ---------> BUTTON_7
    PE3   ---------> BUTTON_8
    */
  /* Configure PE0, PE1, PE2, PE3 GPIO pins in input mode */
  gpio_config.mode            = HAL_GPIO_MODE_INPUT;
  gpio_config.pull            = HAL_GPIO_PULL_UP;
  if (HAL_GPIO_Init(HAL_GPIOE, BUTTON_5_PIN | BUTTON_6_PIN | BUTTON_7_PIN | BUTTON_8_PIN, &gpio_config) != HAL_OK)
  {
    return SYSTEM_PERIPHERAL_ERROR;
  }

  return SYSTEM_OK;
}

system_status_t mx_gpio_default_deinit(void)
{
  /* De-initialize pins of GPIOA port */
  HAL_GPIO_DeInit(HAL_GPIOA, USB_VBUS_PIN);

  /* De-initialize pins of GPIOB port */
  HAL_GPIO_DeInit(HAL_GPIOB, BUTTON_DOWN_PIN | BUTTON_OK_PIN | BUTTON_0_PIN | BUTTON_1_PIN | BUTTON_2_PIN | BUTTON_3_PIN | BUTTON_4_PIN | SW_SEL_PIN | MODE_SEL_PIN);

  /* De-initialize pins of GPIOC port */
  HAL_GPIO_DeInit(HAL_GPIOC, LCD_DC_PIN | LCD_RST_PIN | HAL_GPIO_PIN_8 | HAL_GPIO_PIN_9 | BUTTON_9_PIN);

  /* De-initialize pins of GPIOD port */
  HAL_GPIO_DeInit(HAL_GPIOD, BUTTON_LEFT_PIN | BUTTON_UP_PIN | BUTTON_RIGHT_PIN);

  /* De-initialize pins of GPIOE port */
  HAL_GPIO_DeInit(HAL_GPIOE, BUTTON_5_PIN | BUTTON_6_PIN | BUTTON_7_PIN | BUTTON_8_PIN);

  return SYSTEM_OK;
}
