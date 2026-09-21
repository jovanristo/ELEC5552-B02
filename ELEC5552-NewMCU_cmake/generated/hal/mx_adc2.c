/**
  ******************************************************************************
  * @file           : mx_adc2.c
  * @brief          : ADC2 Peripheral initialization
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
#include "mx_adc2.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype------------------------------------------------*/
/* Exported variables by reference--------------------------------------------*/
static hal_adc_handle_t hADC2;

/******************************************************************************/
/* Exported functions for ADC2 in HAL layer */
/******************************************************************************/
hal_adc_handle_t *mx_adc2_init(void)
{
  if (HAL_ADC_Init(&hADC2, HAL_ADC2) != HAL_OK)
  {
    return NULL;
  }

  HAL_RCC_ADC12_EnableClock();

  if (HAL_RCC_ADCDAC_SetKernelClkSource(HAL_RCC_ADCDAC_CLK_SRC_PSIS) != HAL_OK)
  {
    return NULL;
  }

  hal_adc_config_t adc_config;
  hal_adc_channel_config_t adc_channel_config;

  adc_config.resolution          = HAL_ADC_RESOLUTION_12_BIT;
  adc_config.sampling_mode       = HAL_ADC_SAMPLING_MODE_NORMAL;
  HAL_ADC_SetConfig(&hADC2, &adc_config);

  /****************************************************************************/
  /* Configuration of basic features (mandatory)                              */
  /****************************************************************************/

/* ==================== Group Regular ====================*/
  hal_adc_reg_config_t reg_config;
  reg_config.trigger_src        = HAL_ADC_REG_TRIG_SOFTWARE;
  reg_config.sequencer_length   = 12;
  reg_config.sequencer_discont  = HAL_ADC_REG_SEQ_DISCONT_DISABLE;
  reg_config.continuous         = HAL_ADC_REG_CONV_SINGLE;
  reg_config.overrun            = HAL_ADC_REG_OVR_DATA_OVERWRITTEN;
  HAL_ADC_REG_SetConfig(&hADC2, &reg_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 1;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_9, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 2;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_10, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 3;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_11, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 4;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_13, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 5;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_0, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 6;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_1, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 7;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_2, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 8;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_4, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 9;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_5, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 10;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_6, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 11;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_7, &adc_channel_config);

  adc_channel_config.group           = HAL_ADC_GROUP_REGULAR;
  adc_channel_config.sequencer_rank  = 12;
  adc_channel_config.sampling_time   = HAL_ADC_SAMPLING_TIME_289CYCLES;
  adc_channel_config.input_mode      = HAL_ADC_IN_SINGLE_ENDED;
  HAL_ADC_SetConfigChannel(&hADC2, HAL_ADC_CHANNEL_8, &adc_channel_config);

  /****************************************************************************/
  /* Configuration of additional features (optional)                          */
  /****************************************************************************/

  /* ### ADC2 GPIO Configuration ########################### */
  /* GPIO Clocks activation */
  HAL_RCC_GPIOC_EnableClock();

  HAL_RCC_GPIOB_EnableClock();

  HAL_RCC_GPIOH_EnableClock();

  HAL_RCC_GPIOA_EnableClock();

  hal_gpio_config_t  gpio_config;

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PC4     ------>   ADC2_IN4   ------>  TH_ADC8
       PC5     ------>   ADC2_IN5   ------>  TH_ADC9
       PC1     ------>   ADC2_IN9   ------>  TH_ADC1
       PC2     ------>   ADC2_IN10   ------>  TH_ADC2
       PC3     ------>   ADC2_IN11   ------>  TH_ADC3
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  HAL_GPIO_Init(HAL_GPIOC, TH_ADC8_PIN | TH_ADC9_PIN | TH_ADC1_PIN | TH_ADC2_PIN | TH_ADC3_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PB0     ------>   ADC2_IN6   ------>  TH_ADC10
       PB1     ------>   ADC2_IN7   ------>  TH_ADC11
       PB2     ------>   ADC2_IN8   ------>  THG_ADC12
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  HAL_GPIO_Init(HAL_GPIOB, TH_ADC10_PIN | TH_ADC11_PIN | THG_ADC12_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PH5     ------>   ADC2_IN13   ------>  TH_ADC4
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  HAL_GPIO_Init(TH_ADC4_PORT, TH_ADC4_PIN, &gpio_config);

  /**
    [GPIO Pin] ------> [Signal Name] ------> [Labels]

       PA0     ------>   ADC2_IN0   ------>  TH_ADC5
       PA1     ------>   ADC2_IN1   ------>  TH_ADC6
       PA2     ------>   ADC2_IN2   ------>  TH_ADC7
    **/
  gpio_config.mode        = HAL_GPIO_MODE_ANALOG;
  gpio_config.pull        = HAL_GPIO_PULL_NO;
  HAL_GPIO_Init(HAL_GPIOA, TH_ADC5_PIN | TH_ADC6_PIN | TH_ADC7_PIN, &gpio_config);

  return &hADC2;
}

void mx_adc2_deinit(void)
{
  (void)HAL_ADC_DeInit(&hADC2);

  /* De-initialize all GPIOC pins associated with ADC2 */
  HAL_GPIO_DeInit(HAL_GPIOC, TH_ADC1_PIN | TH_ADC2_PIN | TH_ADC3_PIN | TH_ADC8_PIN | TH_ADC9_PIN);

  /* De-initialize all GPIOB pins associated with ADC2 */
  HAL_GPIO_DeInit(HAL_GPIOB, TH_ADC10_PIN | TH_ADC11_PIN | THG_ADC12_PIN);

  /* De-initialize all GPIOH pins associated with ADC2 */
  HAL_GPIO_DeInit(TH_ADC4_PORT, TH_ADC4_PIN);

  /* De-initialize all GPIOA pins associated with ADC2 */
  HAL_GPIO_DeInit(HAL_GPIOA, TH_ADC5_PIN | TH_ADC6_PIN | TH_ADC7_PIN);
}

hal_adc_handle_t *mx_adc2_gethandle(void)
{
  return &hADC2;
}
