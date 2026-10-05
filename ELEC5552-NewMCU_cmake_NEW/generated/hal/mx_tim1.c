/**
  ******************************************************************************
  * @file           : mx_tim1.c
  * @brief          : Peripheral initialization
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
#include "mx_tim1.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_tim_handle_t hTIM1;

/* Exported function definition ----------------------------------------------*/
/******************************************************************************/
/* Exported functions for TIM1 in HAL layer */
/******************************************************************************/
hal_tim_handle_t *mx_tim1_init(void)
{
  if (HAL_TIM_Init(&hTIM1, HAL_TIM1) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_TIM1_EnableClock();

  /* Timer configuration with external clock */
  hal_tim_config_t config;
  config.prescaler              = 0;
  config.counter_mode           = HAL_TIM_COUNTER_UP;
  config.period                 = 0xFFFF;
  config.repetition_counter     = 0;
  config.clock_sel.clock_source = HAL_TIM_CLK_EXTERNAL_MODE2;
  if (HAL_TIM_SetConfig(&hTIM1, &config) != HAL_OK)
  {
    return NULL;
  }

  /* Configure the external trigger input */
  hal_tim_ext_trig_config_t ext_trig;
  ext_trig.source         = HAL_TIM_EXT_TRIG_TIM1_GPIO;
  ext_trig.polarity       = HAL_TIM_EXT_TRIG_NONINVERTED;
  ext_trig.filter         = HAL_TIM_FDIV1;
  ext_trig.prescaler      = HAL_TIM_EXT_TRIG_DIV1;
  ext_trig.sync_prescaler = HAL_TIM_EXT_TRIG_SYNC_DIV1;
  HAL_TIM_SetExternalTriggerInput(&hTIM1, &ext_trig);

  /* Sampling Clock */
  if (HAL_TIM_SetDTSPrescaler(&hTIM1, HAL_TIM_DTS_DIV1) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_TIM_SetDTS2Prescaler(&hTIM1, HAL_TIM_DTS2_DIV1) != HAL_OK)
  {
    return NULL;
  }

  /* Update Event Management */
  if (HAL_TIM_SetUpdateSource(&hTIM1, HAL_TIM_UPDATE_REGULAR) != HAL_OK)
  {
    return NULL;
  }
  if (HAL_TIM_EnableUpdateGeneration(&hTIM1) != HAL_OK)
  {
    return NULL;
  }
  /* ### TIM1 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOE_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PE7     ------>   TIM1_ETR   ------>  TACH_FAN
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ALTERNATE;
  gpio_config.output_type = HAL_GPIO_OUTPUT_PUSHPULL;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  gpio_config.speed       = HAL_GPIO_SPEED_FREQ_LOW;
  gpio_config.alternate   = HAL_GPIO_AF_1;
  HAL_GPIO_Init(TACH_FAN_PORT, TACH_FAN_PIN, &gpio_config);

  return &hTIM1;
}

void mx_tim1_deinit(void)
{
  (void)HAL_TIM_DeInit(&hTIM1);

  HAL_RCC_TIM1_DisableClock();

  HAL_RCC_TIM1_Reset();

  /* De-initialize all GPIOE pins associated with TIM1 */
  HAL_GPIO_DeInit(TACH_FAN_PORT, TACH_FAN_PIN);
}

hal_tim_handle_t *mx_tim1_gethandle(void)
{
  return &hTIM1;
}
