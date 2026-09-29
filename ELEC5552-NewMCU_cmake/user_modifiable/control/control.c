#include "control.h"

#include "inputs.h"
#include "outputs.h"
#include "protection.h"
#include "timers.h"
#include "logger.h"

#include <stddef.h>


/* =========================================================
 * Default settings
 *
 * These are placeholders until the UI provides values.
 * ========================================================= */

static ControlSettings_t settings =
{
    .mode = CONTROL_MODE_CC,

    .current_setpoint_a = 0.0f,
    .voltage_setpoint_v = 0.0f,

    .external_temp_limit_c = 60.0f,

    .run_hours = 0U,
    .run_minutes = 0U,
    .run_seconds = 0U,

    .sample_period_ms = 100U
};


/* =========================================================
 * Current system state
 * ========================================================= */

static ControlState_t state;


/* =========================================================
 * Private functions
 * ========================================================= */

static void Control_UpdateMeasurements(void);

static void Control_UpdateOutputState(void);

static void Control_UpdateTimerState(void);

static void Control_UpdateSystemState(void);


/* =========================================================
 * Initialise control system
 * ========================================================= */

void Control_Init(void)
{
    state.running = false;


    /* Measurements */
    state.dut_voltage_v = 0.0f;

    state.current_side11_a = 0.0f;
    state.current_side12_a = 0.0f;
    state.current_side21_a = 0.0f;
    state.current_side22_a = 0.0f;

    state.total_current_a = 0.0f;


    /* DAC state */
    state.dac_raw = 0U;
    state.dac_voltage_v = 0.0f;


    /* Timer */
    state.elapsed_ms = 0U;
    state.remaining_ms = 0U;


    /* System state */
    state.protection_ok = true;
    state.logger_running = false;


    /*
     * Always initialise control in the safe state.
     */
    Outputs_SetSafe();
}


/* =========================================================
 * Main control update
 *
 * Call continuously from main().
 * ========================================================= */

void Control_Update(void)
{
    /* -----------------------------------------------------
     * 1. Update physical measurements
     * ----------------------------------------------------- */

    Inputs_Update();


    /* -----------------------------------------------------
     * 2. Copy measurements into control state
     * ----------------------------------------------------- */

    Control_UpdateMeasurements();


    /* -----------------------------------------------------
     * 3. Run protection system
     *
     * Protection uses the latest Inputs_Get() values.
     * ----------------------------------------------------- */

    Protection_Update(
        settings.external_temp_limit_c
    );


    /* -----------------------------------------------------
     * 4. Protection shutdown
     * ----------------------------------------------------- */

    if (!Protection_IsOK())
    {
        /*
         * Immediately force DAC command to zero.
         */
        Outputs_SetSafe();


        /*
         * Stop logging / run timer.
         */
        if (Logger_IsRunning())
        {
            Logger_Stop();
        }


        state.running = false;
    }


    /* -----------------------------------------------------
     * 5. Check test duration
     * ----------------------------------------------------- */

    if (state.running &&
        Timers_RunTimerFinished())
    {
        Control_Stop();
    }


    /* -----------------------------------------------------
     * 6. Update logger
     * ----------------------------------------------------- */

    Logger_Update();


    /* -----------------------------------------------------
     * 7. CC / CV CONTROL
     *
     * DO NOT IMPLEMENT YET.
     *
     * Later this becomes:
     *
     * if (state.running)
     * {
     *     if (settings.mode == CONTROL_MODE_CC)
     *     {
     *         Outputs_SetCurrent(
     *             settings.current_setpoint_a);
     *     }
     *
     *     else
     *     {
     *         Outputs_SetVoltage(
     *             settings.voltage_setpoint_v);
     *     }
     * }
     *
     * For now the DAC remains at its safe value.
     * ----------------------------------------------------- */


    /* -----------------------------------------------------
     * 8. Update information used by UI
     * ----------------------------------------------------- */

    Control_UpdateOutputState();

    Control_UpdateTimerState();

    Control_UpdateSystemState();
}


/* =========================================================
 * Read measurements from inputs.c
 * ========================================================= */

static void Control_UpdateMeasurements(void)
{
    const Inputs_t *inputs =
        Inputs_Get();


    if (inputs == NULL)
    {
        return;
    }


    /* DUT voltage */
    state.dut_voltage_v =
        inputs->dut_voltage_v;


    /* Individual current measurements */
    state.current_side11_a =
        inputs->current_side11_a;

    state.current_side12_a =
        inputs->current_side12_a;

    state.current_side21_a =
        inputs->current_side21_a;

    state.current_side22_a =
        inputs->current_side22_a;


    /* Total measured load current */
    state.total_current_a =
        state.current_side11_a +
        state.current_side12_a +
        state.current_side21_a +
        state.current_side22_a;
}


/* =========================================================
 * Read current DAC command from outputs.c
 * ========================================================= */

static void Control_UpdateOutputState(void)
{
    state.dac_raw =
        Outputs_GetDACRaw();


    state.dac_voltage_v =
        Outputs_GetDACVoltage();
}


/* =========================================================
 * Update timer information
 * ========================================================= */

static void Control_UpdateTimerState(void)
{
    state.elapsed_ms =
        Timers_RunTimerGetElapsedMs();


    state.remaining_ms =
        Timers_RunTimerGetRemainingMs();
}


/* =========================================================
 * Update protection / logging information
 * ========================================================= */

static void Control_UpdateSystemState(void)
{
    state.protection_ok =
        Protection_IsOK();


    state.logger_running =
        Logger_IsRunning();
}


/* =========================================================
 * Select operating mode
 * ========================================================= */

void Control_SetMode(ControlMode_t mode)
{
    if ((mode != CONTROL_MODE_CC) &&
        (mode != CONTROL_MODE_CV))
    {
        return;
    }


    settings.mode = mode;
}


/* =========================================================
 * Set requested CC current
 *
 * This ONLY stores the requested value.
 *
 * It does not command the DAC yet.
 * ========================================================= */

void Control_SetCurrentSetpoint(float current_a)
{
    if ((current_a < 0.0f) ||
        (current_a > OUTPUT_CC_MAX_CURRENT_A))
    {
        return;
    }


    settings.current_setpoint_a =
        current_a;
}


/* =========================================================
 * Set requested CV voltage
 *
 * This ONLY stores the requested value.
 *
 * It does not command the DAC yet.
 * ========================================================= */

void Control_SetVoltageSetpoint(float voltage_v)
{
    if ((voltage_v < 0.0f) ||
        (voltage_v > OUTPUT_CV_MAX_VOLTAGE_V))
    {
        return;
    }


    settings.voltage_setpoint_v =
        voltage_v;
}


/* =========================================================
 * Set external DUT / battery temperature limit
 * ========================================================= */

void Control_SetExternalTempLimit(float temperature_c)
{
    settings.external_temp_limit_c =
        temperature_c;
}


/* =========================================================
 * Set run duration
 * ========================================================= */

void Control_SetRunTime(uint32_t hours,
                        uint32_t minutes,
                        uint32_t seconds)
{
    settings.run_hours = hours;

    settings.run_minutes = minutes;

    settings.run_seconds = seconds;


    Timers_RunTimerSet(
        hours,
        minutes,
        seconds
    );
}


/* =========================================================
 * Set requested logging period
 * ========================================================= */

void Control_SetSamplePeriod(uint32_t period_ms)
{
    if (period_ms == 0U)
    {
        return;
    }


    settings.sample_period_ms =
        period_ms;
}


/* =========================================================
 * Start test
 * ========================================================= */

bool Control_Start(void)
{
    /*
     * Never start while a protection fault exists.
     */
    if (!Protection_IsOK())
    {
        return false;
    }


    /*
     * Configure requested duration.
     */
    Timers_RunTimerSet(
        settings.run_hours,
        settings.run_minutes,
        settings.run_seconds
    );


    /*
     * Logger_Start() also begins the run timer
     * in our current architecture.
     */
    if (!Logger_Start(
            settings.sample_period_ms))
    {
        return false;
    }


    /*
     * Mark system as running.
     */
    state.running = true;


    /*
     * IMPORTANT:
     *
     * Do not apply CC or CV command yet.
     *
     * Actual mode logic will be added later.
     */
    Outputs_SetSafe();


    return true;
}


/* =========================================================
 * Stop test
 * ========================================================= */

void Control_Stop(void)
{
    /*
     * First remove the electronic load command.
     */
    Outputs_SetSafe();


    /*
     * Then stop logger / timer.
     */
    if (Logger_IsRunning())
    {
        Logger_Stop();
    }


    state.running = false;
}


/* =========================================================
 * Return current settings
 * ========================================================= */

const ControlSettings_t *Control_GetSettings(void)
{
    return &settings;
}


/* =========================================================
 * Return current state
 * ========================================================= */

const ControlState_t *Control_GetState(void)
{
    return &state;
}