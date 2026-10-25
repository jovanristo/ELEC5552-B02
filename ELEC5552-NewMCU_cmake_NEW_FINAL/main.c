/**
  ******************************************************************************
  * file           : main.c
  * brief          : Main program body
  *                  Calls target system initialization then loop in main.
  ******************************************************************************
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

#include "inputs.h"
#include "outputs.h"
#include "protection.h"
#include "fan.h"
#include "control.h"
#include "logger.h"
#include "display.h"
#include "app.h"
#include "timers.h"
#include "lvgl.h"
#include "mx_usbx_app.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* Private functions prototype -----------------------------------------------*/

/**
  * brief:  The application entry point.
  * retval: none but we specify int to comply with C99 standard
  */
int main(void)
{
  /** System Init: this code placed in targets folder initializes your system.
    * It calls the initialization (and sets the initial configuration) of the peripherals.
    * You can use STM32CubeMX to generate and call this code or not in this project.
    * It also contains the HAL initialization and the initial clock configuration.
    */
  if (mx_system_init() != SYSTEM_OK)
  {
    return (-1);
  }
  else
  {
    App_Init();

    Inputs_Init();

    if (!Outputs_Init())
    {
        return -1;
    }

    Protection_Init();

    if (!Fan_Init())
    {
        return -1;
    }

    Control_Init();
    if (app_usbx_init() != UX_SUCCESS)
    {
        return -1;
    }


    lv_init();

    // Use the existing firmware millisecond timer.
    lv_tick_set_cb(Timers_GetMillis);

    if (!Display_Init())
    {
        return -1;
    }

    uint32_t last_lvgl_ms = Timers_GetMillis();

    while (1) {

      app_usbx_process();

      Inputs_Update();
      App_Update();

      Protection_Update(45.0f);

      Display_Update();
      Control_Update();

      Fan_Update();
      Logger_Update();
      

      uint32_t now_ms = Timers_GetMillis();

      if ((uint32_t)(now_ms - last_lvgl_ms) >= 5U)
      {
          lv_timer_handler();
          last_lvgl_ms = now_ms;
      }

    }
  }
} /* end main */

