#ifndef APP_H
#define APP_H

#define NUM_STAGES 8U

#include <stdbool.h>
#include <stdint.h>
#include "inputs.h"

typedef struct
{
    /* Measurements */
    float dut_voltage_v;
    float total_current_a;
    float power_w;

    float current_stage_a[NUM_STAGES];
    float temperature_c[NUM_THERMISTORS];

    float fan_duty;
    uint32_t fan_rpm;

    /* User settings */
    float current_setpoint_a;
    float voltage_setpoint_v;

    uint32_t run_time_ms;
    uint32_t sample_period_ms;

    /* Status */
    bool running;
    bool usb_connected;
    bool logging;

} AppState_t;

void App_Init(void);
void App_Update(void);

AppState_t *App_GetState(void);

#endif