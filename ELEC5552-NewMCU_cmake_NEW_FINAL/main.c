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

    bool usb_ok = (app_usbx_init() == UX_SUCCESS);

    lv_init();

    // Use the existing firmware millisecond timer.
    lv_tick_set_cb(Timers_GetMillis);

    bool display_ok = Display_Init();

    uint32_t last_lvgl_ms = Timers_GetMillis();

    bool fallback_started = false;

    while (1)
    {
        if (usb_ok)
        {
            app_usbx_process();
        }

        Inputs_Update();
        App_Update();

        Protection_Update(45.0f);

        if (!display_ok)
        {
            const Inputs_t *in = Inputs_Get();
            AppState_t *app = App_GetState();

            if (in->button_left)
            {
                Control_Stop();
            }
            else if (!fallback_started &&
                    in->button_ok &&
                    app->dut_connected &&
                    Protection_IsOK())
            {
                App_SelectMode(APP_MODE_CC);
                app->current_setpoint_a = 0.5f;
                App_SetRunTime(0, 0, 10);

                if (Control_Start())
                {
                    fallback_started = true;
                }
            }

        }

        if (display_ok)
        {
            Display_Update();
        }

        Control_Update();
        Fan_Update();
        Logger_Update();

        uint32_t now_ms = Timers_GetMillis();

        if (display_ok &&
            (uint32_t)(now_ms - last_lvgl_ms) >= 5U)
        {
            lv_timer_handler();
            last_lvgl_ms = now_ms;
        }
    }
  }
} /* end main */

