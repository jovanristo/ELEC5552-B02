#include "control.h"

#include "app.h"
#include "outputs.h"
#include "protection.h"
#include "timers.h"

#include <stddef.h>
#include <math.h>

//  * state
static ControlState_t control_state;

#define CC_RAMP_RATE_A_PER_S 5.0f

static float cc_command_a = 0.0f;
static uint32_t cc_last_update_ms = 0U;

#define CV_END_CURRENT_A 0.1f
#define CV_END_HOLD_MS 5000U
#define CV_SETTLE_MS 2000U
#define CV_REBOUND_WAIT_MS 1000U
#define CV_START_MARGIN_V 0.3f
#define CV_SWITCH_SETTLE_MS 10U

/* CV soft-start parameters (provisional; validate on hardware). */
#define CV_RAMP_RATE_V_PER_S 0.1f
#define CV_RAMP_UPDATE_MS 50U
#define CV_RAMP_MAX_STEP_MS 100U

static uint32_t cv_last_update_ms = 0U;
static bool cv_startup_enabled = false;
static float cv_reference_v = 0.0f;
static uint32_t cv_switch_start_ms = 0U;

static uint32_t cv_rebound_start_ms = 0U;

static bool cv_auto_stop = false;
static bool cv_low_current_timing = false;

static uint32_t cv_start_ms = 0U;
static uint32_t cv_low_current_start_ms = 0U;


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

            /* Validate DUT voltage and current setpoint */
            if (!isfinite(app->dut_voltage_v) ||
                !isfinite(app->current_setpoint_a) ||
                app->dut_voltage_v <= 0.0f ||
                app->dut_voltage_v > 30.0f ||
                app->current_setpoint_a < 0.0f ||
                app->current_setpoint_a > 30.0f)
            {
                Control_Stop();
                break;
            }

            /* Validate measured current */
            if (!isfinite(app->total_current_a) ||
                app->total_current_a < 0.0f ||
                app->total_current_a >= 30.0f)
            {
                Control_Stop();
                break;
            }

            /* Check four current-sensing channels */
            bool current_valid = true;

            for (uint32_t i = 0U; i < 4U; i++)
            {
                if (!isfinite(app->current_stage_a[i]) ||
                    app->current_stage_a[i] < 0.0f ||
                    app->current_stage_a[i] >= 7.5f)
                {
                    current_valid = false;
                    break;
                }
            }

            if (!current_valid)
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

            float target_current_a = app->current_setpoint_a;

            if (target_current_a > max_current_a)
            {
                target_current_a = max_current_a;
            }

            //current ramp
            uint32_t now_ms = Timers_GetMillis();

            uint32_t elapsed_ms = now_ms - cc_last_update_ms;

            cc_last_update_ms = now_ms;

           
            /* Limit large time gaps between updates */
            if (elapsed_ms > 100U)
            {
                elapsed_ms = 100U;
            }

            /* Calculate the current increment */
            float current_increment_a =
                CC_RAMP_RATE_A_PER_S * ((float)elapsed_ms / 1000.0f);

            /* Gradually increase commanded current */
            cc_command_a += current_increment_a;

            if (cc_command_a > target_current_a)
            {
                cc_command_a = target_current_a;
            }

            /* Divide total current between 8 stages */
            float stage_current_a = cc_command_a / 8.0f;


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
                Outputs_ClampEnable();

                // Record when we disabled the load.
                cv_rebound_start_ms = Timers_GetMillis();

                control_state = CONTROL_STATE_CC_TO_CV;

                break;
            }



            break;
        }

        
        case CONTROL_STATE_CC_TO_CV:
        {
            // Keep MOSFETs disabled during transition.
            Outputs_ClampEnable();

            // Check protection and run timer.
            if (!Protection_IsOK() ||
                Timers_RunTimerFinished())
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

            // Remove the previous CC current command.
            if (!Outputs_SetCurrent(0.0f))
            {
                Control_Stop();
                break;
            }

            // Wait for battery voltage to rebound.
            if (!Timers_HasElapsed(cv_rebound_start_ms,
                                CV_REBOUND_WAIT_MS))
            {
                break;
            }

            // Read recovered battery voltage.
            float rebound_voltage_v = app->dut_voltage_v;
            float target_voltage_v = app->voltage_setpoint_v;

            // Validate voltage measurements and setpoint.
            if (!isfinite(rebound_voltage_v) ||
                !isfinite(target_voltage_v) ||
                rebound_voltage_v <= 0.0f ||
                target_voltage_v <= 0.0f ||
                target_voltage_v > rebound_voltage_v ||
                rebound_voltage_v > (30.0f - CV_START_MARGIN_V))
            {
                Control_Stop();
                break;
            }

            // Set CV reference above recovered DUT voltage.
            cv_reference_v = rebound_voltage_v + CV_START_MARGIN_V;

            // Configure DAC while MOSFETs remain clamped.
            if (!Outputs_SetVoltage(cv_reference_v))
            {
                Control_Stop();
                break;
            }

            // Select CV analogue control path.
            Outputs_SelectCV();
            cv_switch_start_ms = Timers_GetMillis();

            // Battery-discharge CV: terminate once current tapers.
            cv_auto_stop = true;
            cv_low_current_timing = false;
            cv_low_current_start_ms = 0U;
            cv_startup_enabled = false;
            cv_last_update_ms = Timers_GetMillis();

            // Enter CV startup with MOSFETs still clamped.
            control_state = CONTROL_STATE_CV_STARTUP;

            break;
        }



        case CONTROL_STATE_CV_STARTUP:
        {
            // Check protection and run timer.
            if (!Protection_IsOK() ||
                Timers_RunTimerFinished())
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

            float voltage_v = app->dut_voltage_v;
            float target_v = app->voltage_setpoint_v;
            float current_a = app->total_current_a;
            // A voltage-only run requires an externally current-limited PSU.
            // Use 30 A solely as an emergency software abort threshold.
            float requested_a = cv_auto_stop ?
                app->current_setpoint_a : 30.0f;

            // Validate measurements and setpoints.
            if (!isfinite(voltage_v) ||
                !isfinite(target_v) ||
                !isfinite(current_a) ||
                !isfinite(requested_a) ||
                !isfinite(cv_reference_v) ||
                voltage_v <= 0.0f ||
                voltage_v > 30.0f ||
                target_v <= 0.0f ||
                target_v > cv_reference_v ||
                current_a < 0.0f ||
                requested_a <= 0.0f)
            {
                Control_Stop();
                break;
            }

            // Check all four measured MOSFET pairs.
            for (uint32_t i = 0U; i < 4U; i++)
            {
                float pair_current_a = app->current_stage_a[i];

                if (!isfinite(pair_current_a) ||
                    pair_current_a < 0.0f ||
                    pair_current_a >= 7.5f)
                {
                    Control_Stop();
                    break;
                }
            }

            // Stop if any protection check has disabled the load.
            if (control_state != CONTROL_STATE_CV_STARTUP)
            {
                break;
            }

            // Determine allowed current.
            float max_current_a = requested_a;

            if (max_current_a > 30.0f)
            {
                max_current_a = 30.0f;
            }

            float power_current_limit_a = 500.0f / voltage_v;

            if (max_current_a > power_current_limit_a)
            {
                max_current_a = power_current_limit_a;
            }

            // Stop on measured current or power limit.
            if (current_a >= max_current_a ||
                current_a * voltage_v >= 500.0f)
            {
                Control_Stop();
                break;
            }

            /* Allow analogue reference and MUX to settle */
            if (!Timers_HasElapsed(cv_switch_start_ms,
                                CV_SWITCH_SETTLE_MS))
            {
                break;
            }

            // First startup update: release the clamp.
            if (!cv_startup_enabled)
            {
                float safe_initial_v =
                    voltage_v + CV_START_MARGIN_V;

                if (safe_initial_v > 30.0f)
                {
                    Control_Stop();
                    break;
                }

                // Account for any further voltage rebound.
                if (cv_reference_v < safe_initial_v)
                {
                    cv_reference_v = safe_initial_v;

                    if (!Outputs_SetVoltage(cv_reference_v))
                    {
                        Control_Stop();
                        break;
                    }
                }

                cv_last_update_ms = Timers_GetMillis();
                cv_startup_enabled = true;

                Outputs_ClampDisable();
                break;
            }

            // Calculate elapsed time.
            uint32_t now_ms = Timers_GetMillis();
            uint32_t elapsed_ms = now_ms - cv_last_update_ms;

            if (elapsed_ms < CV_RAMP_UPDATE_MS)
            {
                break;
            }

            cv_last_update_ms = now_ms;

            if (elapsed_ms > CV_RAMP_MAX_STEP_MS)
            {
                elapsed_ms = CV_RAMP_MAX_STEP_MS;
            }

            // Pause ramp when approaching current limit.
            if (current_a >= 0.9f * max_current_a)
            {
                break;
            }

            // Gradually decrease the CV reference.
            float voltage_step_v =
                CV_RAMP_RATE_V_PER_S *
                ((float)elapsed_ms / 1000.0f);

            cv_reference_v -= voltage_step_v;

            // Never decrease below requested voltage.
            if (cv_reference_v < target_v)
            {
                cv_reference_v = target_v;
            }

            // Update the DAC reference.
            if (!Outputs_SetVoltage(cv_reference_v))
            {
                Control_Stop();
                break;
            }

            // Target voltage reference reached.
            if (cv_reference_v <= target_v)
            {
                cv_low_current_timing = false;
                cv_low_current_start_ms = 0U;
                cv_start_ms = now_ms;

                control_state = CONTROL_STATE_CV;
            }

            break;
        }



        /* -------------------------------------------------
         * CONSTANT VOLTAGE
         * ------------------------------------------------- */
        
        case CONTROL_STATE_CV:
        {
            // Check protection and run timer.
            if (!Protection_IsOK() ||
                Timers_RunTimerFinished())
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

            float voltage_v = app->dut_voltage_v;
            float current_a = app->total_current_a;
            float target_v = app->voltage_setpoint_v;

            // Validate measurements and setpoint.
            if (!isfinite(voltage_v) ||
                !isfinite(current_a) ||
                !isfinite(target_v) ||
                voltage_v < 0.0f ||
                voltage_v > 30.0f ||
                current_a < 0.0f ||
                target_v <= 0.0f ||
                target_v > 30.0f)
            {
                Control_Stop();
                break;
            }

            // Check the four current-sensing pairs.
            for (uint32_t i = 0U; i < 4U; i++)
            {
                float pair_a = app->current_stage_a[i];

                if (!isfinite(pair_a) ||
                    pair_a < 0.0f ||
                    pair_a >= 7.5f)
                {
                    Control_Stop();
                    break;
                }
            }

            if (control_state != CONTROL_STATE_CV)
            {
                break;
            }

            // Battery discharge retains its user-requested current limit.
            // In standalone CV, 30 A is only a software abort threshold:
            // an external supply must provide its own current limit.
            float allowed_current_a = 30.0f;

            if (cv_auto_stop)
            {
                float requested_a = app->current_setpoint_a;

                if (!isfinite(requested_a) || requested_a <= 0.0f)
                {
                    Control_Stop();
                    break;
                }

                if (requested_a < allowed_current_a)
                {
                    allowed_current_a = requested_a;
                }
            }

            if (current_a >= allowed_current_a ||
                (current_a * voltage_v) >= 500.0f)
            {
                Control_Stop();
                break;
            }

            // Maintain the requested voltage reference.
            if (!Outputs_SetVoltage(target_v))
            {
                Control_Stop();
                break;
            }

            // Standalone CV does not automatically terminate.
            if (!cv_auto_stop)
            {
                break;
            }

            uint32_t now_ms = Timers_GetMillis();

            // Allow current to settle after CV startup.
            if (!Timers_HasElapsed(cv_start_ms,
                                CV_SETTLE_MS))
            {
                cv_low_current_timing = false;
                break;
            }

            // Check whether discharge current is low.
            if (current_a <= CV_END_CURRENT_A)
            {
                // Start timing when current first falls low.
                if (!cv_low_current_timing)
                {
                    cv_low_current_timing = true;
                    cv_low_current_start_ms = now_ms;
                }

                // Stop once current has remained low.
                else if (Timers_HasElapsed(
                            cv_low_current_start_ms,
                            CV_END_HOLD_MS))
                {
                    Control_Stop();
                }
            }
            else
            {
                // Current increased again: reset timer.
                cv_low_current_timing = false;
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

bool Control_Start(void)
{
    AppState_t *app = App_GetState();

    if (app == NULL)
    {
        return false;
    }

    /* Only start from IDLE */
    if (control_state != CONTROL_STATE_IDLE)
    {
        return false;
    }

    /* Require a valid DUT voltage */
    if (!app->dut_connected)
    {
        return false;
    }

    /* Check protection before starting */
    if (!Protection_IsOK())
    {
        return false;
    }

    /* Validate user setpoints */
    float current_a = app->current_setpoint_a;
    float voltage_v = app->voltage_setpoint_v;

    if (!isfinite(current_a) ||
        !isfinite(voltage_v) ||
        current_a < 0.0f ||
        current_a > 30.0f ||
        voltage_v < 0.0f ||
        voltage_v > 30.0f)
    {
        return false;
    }

    /* Explicit CC selection */
    if (app->selected_mode == APP_MODE_CC)
    {
        if (current_a <= 0.0f)
        {
            return false;
        }

        if (voltage_v > 0.0f &&
            voltage_v > app->max_voltage_v)
        {
            return false;
        }

        return Control_StartCC();
    }

    /* Explicit CV selection */
    if (app->selected_mode == APP_MODE_CV)
    {
        if (voltage_v <= 0.0f ||
            voltage_v > app->max_voltage_v)
        {
            return false;
        }

        return Control_StartCV();
    }

    /* No mode selected */
    return false;
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

    if (!isfinite(requested_current_a) ||
        requested_current_a <= 0.0f ||
        requested_current_a > app->max_current_a)
    {
        return false;
    }

    // Keep MOSFETs disabled.
    Outputs_ClampEnable();

    // Select constant current mode.
    Outputs_SelectCC();

    // Start with a zero-current DAC command.
    if (!Outputs_SetCurrent(0.0f))
    {
        Outputs_SetSafe();
        return false;
    }

    /* Configure timer from user-selected duration */
    uint32_t total_seconds = app->run_time_ms / 1000U;

    Timers_RunTimerSet(
        total_seconds / 3600U,
        (total_seconds % 3600U) / 60U,
        total_seconds % 60U
    );

    /* Start run timer */
    if (!Timers_RunTimerStart())
    {
        Outputs_SetSafe();
        return false;
    }

    // Initialise the soft-start ramp.
    cc_command_a = 0.0f;
    cc_last_update_ms = Timers_GetMillis();

    // Enter CC mode.
    control_state = CONTROL_STATE_CC;

    // Release the MOSFET clamp.
    Outputs_ClampDisable();

    app->running = true;

    return true;
}


/* =========================================================
 * Start CV
 * ========================================================= */

bool Control_StartCV(void)
{
    if (control_state != CONTROL_STATE_IDLE ||
        !Protection_IsOK())
    {
        return false;
    }

    AppState_t *app = App_GetState();

    if (app == NULL)
    {
        return false;
    }

    float dut_voltage_v = app->dut_voltage_v;
    float target_v = app->voltage_setpoint_v;

    // Standalone CV requires a connected, externally
    // current-limited supply and a target below its voltage.
    if (!isfinite(dut_voltage_v) ||
        !isfinite(target_v) ||
        dut_voltage_v <= 0.0f ||
        dut_voltage_v > (30.0f - CV_START_MARGIN_V) ||
        target_v <= 0.0f ||
        target_v >= dut_voltage_v)
    {
        return false;
    }

    float initial_reference_v =
        dut_voltage_v + CV_START_MARGIN_V;

    Outputs_ClampEnable();
    Outputs_SelectCV();

    if (!Outputs_SetVoltage(initial_reference_v))
    {
        Outputs_SetSafe();
        return false;
    }

    /* Configure timer from user-selected duration */
    uint32_t total_seconds = app->run_time_ms / 1000U;

    Timers_RunTimerSet(
        total_seconds / 3600U,
        (total_seconds % 3600U) / 60U,
        total_seconds % 60U
    );

    /* Start run timer */
    if (!Timers_RunTimerStart())
    {
        Outputs_SetSafe();
        return false;
    }

    cv_reference_v = initial_reference_v;
    cv_last_update_ms = Timers_GetMillis();
    cv_startup_enabled = false;
    cv_auto_stop = false;  // No automatic current-taper termination.
    cv_low_current_timing = false;
    cv_low_current_start_ms = 0U;

    // CV_STARTUP checks measurements before releasing the clamp.
    control_state = CONTROL_STATE_CV_STARTUP;
    app->running = true;

    return true;
}


/* =========================================================
 * Stop run
 * ========================================================= */

void Control_Stop(void)
{
    Outputs_SetSafe();


    cc_command_a = 0.0f;

    cv_reference_v = 0.0f;
    cv_auto_stop = false;
    cv_startup_enabled = false;
    cv_low_current_timing = false;

    cv_start_ms = 0U;
    cv_last_update_ms = 0U;
    cv_rebound_start_ms = 0U;
    cv_low_current_start_ms = 0U;


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