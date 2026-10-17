#include "app.h"

#include "inputs.h"
#include "fan.h"
#include "usb.h"
#include "logger.h"

#include <math.h>
#include <stddef.h>

static AppState_t app_state;


void App_Init(void)
{
    app_state.selected_mode = APP_MODE_NONE;
    app_state.current_setpoint_a = 0.0f;
    app_state.voltage_setpoint_v = 0.0f;
    app_state.max_current_a = 0.0f;
    app_state.max_voltage_v = 0.0f;
    app_state.dut_connected = false;
    app_state.dut_lost_during_run = false;
    app_state.external_probe_count = 0U;

    app_state.run_time_ms = 60000U;
    app_state.sample_period_ms = 100U;

    app_state.running = false;
    app_state.logging = false;
}


void App_Update(void)
{
    const Inputs_t *inputs = Inputs_Get();

    if (inputs != NULL)
    {
        app_state.dut_voltage_v = inputs->dut_voltage_v;

        app_state.current_stage_a[0] = inputs->current_side11_a;
        app_state.current_stage_a[1] = inputs->current_side12_a;
        app_state.current_stage_a[2] = inputs->current_side21_a;
        app_state.current_stage_a[3] = inputs->current_side22_a;

        for (uint32_t i = 0U; i < NUM_THERMISTORS; i++)
        {
            app_state.temperature_c[i] =
                inputs->thermistor_temp_c[i];
        }

        app_state.total_current_a =
            app_state.current_stage_a[0] +
            app_state.current_stage_a[1] +
            app_state.current_stage_a[2] +
            app_state.current_stage_a[3];

        app_state.power_w =
            app_state.dut_voltage_v *
            app_state.total_current_a;
    }


        /* Calculate operating limits from DUT voltage */

    float voltage_v = app_state.dut_voltage_v;

    /* Provisional valid DUT voltage range */
    app_state.dut_connected =
        isfinite(voltage_v) &&
        voltage_v >= 0.5f &&
        voltage_v <= 30.0f;

    if (app_state.dut_connected)
    {
        /* Maximum current based on 500 W */
        float max_current = 500.0f / voltage_v;

        /* Hardware maximum is 30 A */
        if (max_current > 30.0f)
        {
            max_current = 30.0f;
        }

        /* Round down to 10 mA increments */
        app_state.max_current_a =
            floorf(max_current * 100.0f) / 100.0f;

        /*
        * Maximum voltage setpoint:
        * 0.1 V below measured DUT voltage.
        */
        float max_voltage =
            floorf(voltage_v * 10.0f) / 10.0f
            - 0.1f;

        if (max_voltage < 0.0f)
        {
            max_voltage = 0.0f;
        }

        app_state.max_voltage_v = max_voltage;
    }
    else
    {
        app_state.max_current_a = 0.0f;
        app_state.max_voltage_v = 0.0f;
    }

    app_state.fan_duty = Fan_GetDuty();
    app_state.fan_rpm = Fan_GetRPM();

    app_state.usb_connected = USB_IsConnected();

    app_state.logging = Logger_IsRunning();
}


AppState_t *App_GetState(void)
{
    return &app_state;
}


bool App_SelectMode(AppMode_t mode)
{
    // Mode cannot change while running.
    if (app_state.running)
    {
        return false;
    }

    // Only allow CC or CV selection.
    if (mode != APP_MODE_CC &&
        mode != APP_MODE_CV)
    {
        return false;
    }

    // Record selected operating mode.
    app_state.selected_mode = mode;

    // Clear previous setpoints.
    app_state.current_setpoint_a = 0.0f;
    app_state.voltage_setpoint_v = 0.0f;

    return true;
}


bool App_SetRunTime(uint32_t hours,
                    uint32_t minutes,
                    uint32_t seconds)
{
    /* Timer cannot be changed during a run */
    if (app_state.running)
    {
        return false;
    }

    /* Validate HH:MM:SS */
    if (hours > 24U ||
        minutes > 59U ||
        seconds > 59U)
    {
        return false;
    }

    /* Maximum duration is exactly 24 hours */
    if (hours == 24U &&
        (minutes != 0U || seconds != 0U))
    {
        return false;
    }

    uint32_t total_seconds =
        hours * 3600U +
        minutes * 60U +
        seconds;

    /* Zero duration is not permitted */
    if (total_seconds == 0U)
    {
        return false;
    }

    app_state.run_time_ms = total_seconds * 1000U;

    return true;
}


/* Physical external thermistors 4-7 are ADC indexes 3-6. */
bool App_ExternalProbesReady(void)
{
    const Inputs_t *in = Inputs_Get();
    if (in == NULL || app_state.external_probe_count > 4U) return false;
    for (uint8_t i = 0U; i < app_state.external_probe_count; ++i)
    {
        uint8_t idx = (uint8_t)(i + 3U);
        if (!in->thermistor_valid[idx] ||
            !isfinite(in->thermistor_temp_c[idx]) ||
            in->thermistor_temp_c[idx] >= 45.0f)
            return false;
    }
    return true;
}
