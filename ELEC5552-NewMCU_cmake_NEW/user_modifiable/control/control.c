#include "control.h"

#include "app.h"
#include "outputs.h"
#include "protection.h"
#include "timers.h"

#include <stddef.h>

//  * state
static ControlState_t control_state;


/* =========================================================
 * Initialise state machine
 * ========================================================= */

void Control_Init(void)
{
    Outputs_SetSafe();

    control_state = CONTROL_STATE_IDLE;

     AppState_t *app = App_GetState();

    if (app != NULL)
    {
        app->running = false;
    }
}


/* =========================================================
 * Main state machine
 * ========================================================= */

void Control_Update(void)
{
    switch (control_state)
    {
        /* -------------------------------------------------
         * IDLE
         * ------------------------------------------------- */
        case CONTROL_STATE_IDLE:
        {
            Outputs_SetSafe();


            break;
        }


        /* -------------------------------------------------
         * CONSTANT CURRENT
         * ------------------------------------------------- */
        case CONTROL_STATE_CC:
        {

            if (!Protection_IsOK())
            {
                Control_Stop();
                break;
            }

            if (Timers_RunTimerFinished())
            {
                Control_Stop();
                break;
            }

            AppState_t *app = App_GetState();

            if (app == NULL)
            {
                Control_Stop();
                break;
            }

            float max_current_a;

            if (app->dut_voltage_v > 0.0f)
            {
                max_current_a = 500.0f / app->dut_voltage_v;

                if (max_current_a > 30.0f)
                {
                    max_current_a = 30.0f;
                }
            }
            else
            {
                max_current_a = 0.0f;
            }

            if (app->current_setpoint_a > max_current_a)
            {
                app->current_setpoint_a = max_current_a;
            }

            float stage_current_a =
                app->current_setpoint_a / 8.0f;

            if (!Outputs_SetCurrent(stage_current_a))
            {
                Control_Stop();
                break;
            }

            /*
            * If a voltage target has been set and the DUT has
            * discharged down to that voltage, transition to CV.
            */
            if ((app->voltage_setpoint_v > 0.0f) &&
                (app->dut_voltage_v <= app->voltage_setpoint_v))
            {
                control_state = CONTROL_STATE_CC_TO_CV;
                break;
            }


            break;
        }

        case CONTROL_STATE_CC_TO_CV:
        {
            AppState_t *app = App_GetState();

            if (app == NULL)
            {
                Control_Stop();
                break;
            }

            /*
            * Safely disable the load before changing mode.
            */
            Outputs_ClampEnable();

            /*
            * Remove CC command.
            */
            if (!Outputs_SetCurrent(0.0f))
            {
                Control_Stop();
                break;
            }

            /*
            * Switch analogue control path to CV.
            */
            Outputs_SelectCV();

            /*
            * Set the requested CV voltage.
            */
            if (!Outputs_SetVoltage(app->voltage_setpoint_v))
            {
                Control_Stop();
                break;
            }

            /*
            * Final safety check before enabling CV.
            */
            if (!Protection_IsOK())
            {
                Control_Stop();
                break;
            }

            /*
            * Re-enable the load in CV mode.
            */
            Outputs_ClampDisable();

            control_state = CONTROL_STATE_CV;

            break;
        }


        /* -------------------------------------------------
         * CONSTANT VOLTAGE
         * ------------------------------------------------- */
        case CONTROL_STATE_CV:
        {

            if (!Protection_IsOK())
            {
                Control_Stop();
                break;
            }

            if (Timers_RunTimerFinished())
            {
                Control_Stop();
                break;
            }

            AppState_t *app = App_GetState();

            if (app == NULL)
            {
                Control_Stop();
                break;
            }

            if (!Outputs_SetVoltage(app->voltage_setpoint_v))
            {
                Control_Stop();
                break;
            }



            break;
        }


        /* -------------------------------------------------
         * Invalid state
         * ------------------------------------------------- */
        default:
        {
            control_state = CONTROL_STATE_IDLE;

            break;
        }
    }
}


/* =========================================================
 * Start CC
 * ========================================================= */

bool Control_StartCC(void)
{
    if (control_state != CONTROL_STATE_IDLE)
    {
        return false;
    }

    if (!Protection_IsOK())
    {
        return false;
    }

    AppState_t *app = App_GetState();

    if (app == NULL)
    {
        return false;
    }

    float dut_voltage_v = app->dut_voltage_v;
    float requested_current_a = app->current_setpoint_a;

    if (dut_voltage_v <= 0.0f)
    {
        return false;
    }

    float max_current_a = 500.0f / dut_voltage_v;

    if (max_current_a > 30.0f)
    {
        max_current_a = 30.0f;
    }

    if ((requested_current_a < 0.0f) ||
        (requested_current_a > max_current_a))
    {
        return false;
    }

    float stage_current_a =
        app->current_setpoint_a / 8.0f;

    /*
     * Keep load clamped while configuring.
     */
    Outputs_ClampEnable();

    /*
     * Select CC analogue path.
     */
    Outputs_SelectCC();

    /*
     * Configure DAC for requested current.
     */
    if (!Outputs_SetCurrent(stage_current_a))
    {
        Outputs_SetSafe();
        return false;
    }

    /*
     * DAC and MUX are now configured.
     * Allow MOSFET control loop to operate.
     */
    Outputs_ClampDisable();

    if (!Timers_RunTimerStart())
    {
        Outputs_SetSafe();
        return false;
    }

    control_state = CONTROL_STATE_CC;

    control_state = CONTROL_STATE_CC;
    app->running = true;

    return true;

    return true;
}


/* =========================================================
 * Start CV
 * ========================================================= */

bool Control_StartCV(void)
{
    if (control_state != CONTROL_STATE_IDLE)
    {
        return false;
    }

    if (!Protection_IsOK())
    {
        return false;
    }

    AppState_t *app = App_GetState();

    if (app == NULL)
    {
        return false;
    }

    if ((app->voltage_setpoint_v < 0.0f) ||
        (app->voltage_setpoint_v > 30.0f))
    {
        return false;
    }

    /*
     * Keep load disabled while configuring.
     */
    Outputs_ClampEnable();

    /*
     * Select CV analogue path.
     */
    Outputs_SelectCV();

    /*
     * Set desired CV reference.
     */
    if (!Outputs_SetVoltage(app->voltage_setpoint_v))
    {
        Outputs_SetSafe();
        return false;
    }

    /*
     * Enable the load.
     */
    Outputs_ClampDisable();

    if (!Timers_RunTimerStart())
    {
        Outputs_SetSafe();
        return false;
    }

    control_state = CONTROL_STATE_CV;
    app->running = true;

    return true;

    return true;
}


/* =========================================================
 * Stop run
 * ========================================================= */

void Control_Stop(void)
{
    Outputs_SetSafe();

    Timers_RunTimerStop();

    control_state = CONTROL_STATE_IDLE;

    AppState_t *app = App_GetState();

    if (app != NULL)
    {
        app->running = false;
    }
}


/* =========================================================
 * Get current state
 * ========================================================= */

ControlState_t Control_GetState(void)
{
    return control_state;
}