#ifndef APP_H
#define APP_H

#include <stdbool.h>
#include <stdint.h>

#include "inputs.h"

#define NUM_STAGES       8U

typedef enum
{
    APP_MODE_NONE = 0,
    APP_MODE_CC,
    APP_MODE_CV
    
} AppMode_t;


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
    AppMode_t selected_mode;

    float current_setpoint_a;
    float voltage_setpoint_v;

    /* Calculated operating limits */
    float max_current_a;
    float max_voltage_v;

    bool dut_connected;
    bool dut_lost_during_run;
    uint8_t external_probe_count;  /* 0-4: first physical external probes */

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

bool App_SelectMode(AppMode_t mode);

/* Refuse START when any requested external probe is missing or too hot. */
bool App_ExternalProbesReady(void);

bool App_SetRunTime(uint32_t hours,
                    uint32_t minutes,
                    uint32_t seconds);

#endif