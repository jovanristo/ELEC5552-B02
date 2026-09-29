#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>
#include <stdint.h>


/* =========================================================
 * Operating mode
 * ========================================================= */

typedef enum
{
    CONTROL_MODE_CC = 0,
    CONTROL_MODE_CV

} ControlMode_t;


/* =========================================================
 * User settings
 *
 * These will eventually be modified by the UI.
 * ========================================================= */

typedef struct
{
    /* Selected operating mode */
    ControlMode_t mode;


    /* CC requested current */
    float current_setpoint_a;


    /* CV requested DUT voltage */
    float voltage_setpoint_v;


    /* External DUT / battery temperature cutoff */
    float external_temp_limit_c;


    /* Requested test duration */
    uint32_t run_hours;
    uint32_t run_minutes;
    uint32_t run_seconds;


    /* Requested logging period */
    uint32_t sample_period_ms;

} ControlSettings_t;


/* =========================================================
 * Current control-system state
 *
 * This contains useful information for the UI.
 * ========================================================= */

typedef struct
{
    /* Is a test currently running? */
    bool running;


    /* -----------------------------------------------------
     * Electrical measurements
     * ----------------------------------------------------- */

    float dut_voltage_v;

    float current_side11_a;
    float current_side12_a;
    float current_side21_a;
    float current_side22_a;

    float total_current_a;


    /* -----------------------------------------------------
     * Current DAC command
     * ----------------------------------------------------- */

    uint16_t dac_raw;
    float dac_voltage_v;


    /* -----------------------------------------------------
     * Timer
     * ----------------------------------------------------- */

    uint32_t elapsed_ms;
    uint32_t remaining_ms;


    /* -----------------------------------------------------
     * System status
     * ----------------------------------------------------- */

    bool protection_ok;
    bool logger_running;

} ControlState_t;


/* =========================================================
 * Initialisation / update
 * ========================================================= */

void Control_Init(void);

void Control_Update(void);


/* =========================================================
 * Settings
 * ========================================================= */

void Control_SetMode(ControlMode_t mode);

void Control_SetCurrentSetpoint(float current_a);

void Control_SetVoltageSetpoint(float voltage_v);

void Control_SetExternalTempLimit(float temperature_c);

void Control_SetRunTime(uint32_t hours,
                        uint32_t minutes,
                        uint32_t seconds);

void Control_SetSamplePeriod(uint32_t period_ms);


/* =========================================================
 * Run control
 * ========================================================= */

bool Control_Start(void);

void Control_Stop(void);


/* =========================================================
 * State access
 * ========================================================= */

const ControlSettings_t *Control_GetSettings(void);

const ControlState_t *Control_GetState(void);


#endif /* CONTROL_H */