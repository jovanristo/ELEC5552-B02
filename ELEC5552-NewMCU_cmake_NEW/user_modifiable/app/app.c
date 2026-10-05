#include "app.h"

#include "inputs.h"
#include "fan.h"
#include "usb.h"
#include "logger.h"

static AppState_t app_state;


void App_Init(void)
{
    app_state.current_setpoint_a = 0.0f;
    app_state.voltage_setpoint_v = 0.0f;

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

    app_state.fan_duty = Fan_GetDuty();
    app_state.fan_rpm = Fan_GetRPM();

    app_state.usb_connected = USB_IsConnected();

    app_state.logging = Logger_IsRunning();
}


AppState_t *App_GetState(void)
{
    return &app_state;
}