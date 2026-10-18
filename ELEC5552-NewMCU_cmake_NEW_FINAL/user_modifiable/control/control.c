#include "control.h"

#include "app.h"
#include "outputs.h"
#include "protection.h"
#include "timers.h"
#include "user_limits.h"

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
/* Absolute measured limits for all operating modes. */
#define CV_POWER_LIMIT_W 510.0f
#define CV_TOTAL_CURRENT_LIMIT_A 30.0f
/* Each measured channel represents TWO MOSFETs, not one. */
#define CV_PAIR_CURRENT_LIMIT_A 7.5f
/* Begin adjusting the CV reference before a measured hard limit is reached.
 * This is an operating headroom, not a higher permitted rating. */
#define CV_BACKOFF_START_FRACTION 0.98f
#define CV_BACKOFF_STEP_V 0.02f
#define CV_MAX_REFERENCE_V 30.0f

static uint32_t cv_last_update_ms = 0U;
static bool cv_startup_enabled = false;
static float cv_reference_v = 0.0f;
static uint32_t cv_switch_start_ms = 0U;

static uint32_t cv_rebound_start_ms = 0U;

static bool cv_auto_stop = false;
static bool cv_low_current_timing = false;

static uint32_t cv_start_ms = 0U;
static uint32_t cv_low_current_start_ms = 0U;

/* Increase CV reference to reduce the demanded current.
 * This sampled-data adjustment cannot prevent fast analogue transients.
 */
static bool Control_CVBackOff(void)
{
    if (!isfinite(cv_reference_v) ||
        cv_reference_v >= CV_MAX_REFERENCE_V)
    {
        return false;
    }

    cv_reference_v += CV_BACKOFF_STEP_V;

    if (cv_reference_v > CV_MAX_REFERENCE_V)
    {
        cv_reference_v = CV_MAX_REFERENCE_V;
    }

    return Outputs_SetVoltage(cv_reference_v);
}

/* CV supervisory limits, using the four TWO-MOSFET pair measurements.
 * Returns false at an absolute limit or invalid measurement (shutdown).
 * Below the absolute limits, back_off is true when any limit is close.
 */
static bool Control_CVCheckLimits(const AppState_t *app,
                                  float voltage_v,
                                  float total_current_a,
                                  bool *back_off)
{
    if (app == NULL || back_off == NULL ||
        !isfinite(voltage_v) || voltage_v <= 0.0f ||
        !isfinite(total_current_a) || total_current_a < 0.0f)
    {
        return false;
    }

    float power_w = voltage_v * total_current_a;

    if (!isfinite(power_w) ||
        power_w >= CV_POWER_LIMIT_W ||
        total_current_a >= CV_TOTAL_CURRENT_LIMIT_A)
    {
        return false;
    }

    *back_off = (power_w >= CV_POWER_LIMIT_W * CV_BACKOFF_START_FRACTION) ||
                (total_current_a >= CV_TOTAL_CURRENT_LIMIT_A * CV_BACKOFF_START_FRACTION);

    for (uint32_t i = 0U; i < 4U; i++)
    {
        float pair_current_a = app->current_stage_a[i];

        if (!isfinite(pair_current_a) ||
            pair_current_a < 0.0f ||
            pair_current_a >= CV_PAIR_CURRENT_LIMIT_A)
        {
            return false;
        }

        if (pair_current_a >= CV_PAIR_CURRENT_LIMIT_A * CV_BACKOFF_START_FRACTION)
        {
            *back_off = true;
        }
    }

    return true;
}


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

            if (!current_valid ||
                (app->dut_voltage_v * app->total_current_a) >= CV_POWER_LIMIT_W)
            {
                /* CC also uses a measured 500 W hard shutdown. */
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
             * User band (REQ-10): measured current must stay
             * within +/-1 A of the commanded current.
             *
             * The band is centred on cc_command_a (ramped and
             * power-capped), so it follows the soft-start ramp
             * and the 500 W cap without causing a stop.
             * A change in the user setpoint re-arms the band.
             */
            if (UserLimits_Update(app->current_setpoint_a,
                                  cc_command_a,
                                  USER_LIMIT_CURRENT_BAND_A,
                                  app->total_current_a,
                                  now_ms) != USER_LIMIT_OK)
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
                rebound_voltage_v > CV_MAX_REFERENCE_V)
            {
                Control_Stop();
                break;
            }

            // Set CV reference above recovered DUT voltage.
            cv_reference_v = rebound_voltage_v + CV_START_MARGIN_V;
            if (cv_reference_v > CV_MAX_REFERENCE_V)
            {
                // At 30 V, the full startup margin is not available.
                cv_reference_v = CV_MAX_REFERENCE_V;
            }

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
            // CV uses the 500 W measured-power ceiling; current-sensing
            // channels and the 30 A check remain separate safety checks.

            // Validate measurements and setpoints.
            if (!isfinite(voltage_v) ||
                !isfinite(target_v) ||
                !isfinite(current_a) ||
                !isfinite(cv_reference_v) ||
                voltage_v <= 0.0f ||
                voltage_v > 30.0f ||
                target_v <= 0.0f ||
                target_v > cv_reference_v ||
                current_a < 0.0f)
            {
                Control_Stop();
                break;
            }

            // All three CV limits: 500 W total, 30 A total, 7.5 A per pair.
            bool cv_back_off = false;
            if (!Control_CVCheckLimits(app, voltage_v, current_a, &cv_back_off))
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

                if (safe_initial_v > CV_MAX_REFERENCE_V)
                {
                    safe_initial_v = CV_MAX_REFERENCE_V;
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

            // Approach any limit: raise CV reference to reduce demand.
            // At the absolute limit Control_CVCheckLimits has already stopped.
            if (cv_back_off)
            {
                if (!Control_CVBackOff())
                {
                    Control_Stop();
                }
                break;
            }

            // Otherwise, continue slowly towards the requested voltage.
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

                /*
                 * Start the voltage band (REQ-10) now that the
                 * reference has reached the target. It arms once
                 * the DUT voltage enters target +/-1 V.
                 * Also restarts the band after a CV back-off.
                 */
                UserLimits_Start(target_v, now_ms);

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
                target_v > app->max_voltage_v)
            {
                Control_Stop();
                break;
            }

            // Apply the same limits in steady-state CV as in CV_STARTUP.
            bool cv_back_off = false;
            if (!Control_CVCheckLimits(app, voltage_v, current_a, &cv_back_off))
            {
                Control_Stop();
                break;
            }

            // If any limit is approaching, back off and re-enter the ramp.
            // Do not force the user target while current/power is constrained.
            if (cv_back_off)
            {
                if (!Control_CVBackOff())
                {
                    Control_Stop();
                    break;
                }

                cv_last_update_ms = Timers_GetMillis();
                cv_startup_enabled = true; // Clamp already released.
                cv_low_current_timing = false;
                cv_low_current_start_ms = 0U;
                control_state = CONTROL_STATE_CV_STARTUP;
                break;
            }

            // No limit approaching: maintain the requested voltage reference.
            if (!Outputs_SetVoltage(target_v))
            {
                Control_Stop();
                break;
            }

            /*
             * User band (REQ-10): DUT voltage must stay within
             * target +/-1 V once it has settled.
             */
            if (UserLimits_Update(target_v,
                                  target_v,
                                  USER_LIMIT_VOLTAGE_BAND_V,
                                  voltage_v,
                                  Timers_GetMillis()) != USER_LIMIT_OK)
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

    /*
     * Start the current band (REQ-10). It follows the
     * soft-start ramp and arms once the current is in band.
     */
    UserLimits_Start(app->current_setpoint_a, cc_last_update_ms);

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
        dut_voltage_v > CV_MAX_REFERENCE_V ||
        target_v <= 0.0f ||
        target_v >= dut_voltage_v)
    {
        return false;
    }

    float initial_reference_v =
        dut_voltage_v + CV_START_MARGIN_V;
    if (initial_reference_v > CV_MAX_REFERENCE_V)
    {
        // Full startup margin cannot be provided at the 30 V limit.
        initial_reference_v = CV_MAX_REFERENCE_V;
    }

    /* Keep MOSFETs disabled during configuration */
    Outputs_ClampEnable();

    /* Configure CV reference before selecting CV */
    if (!Outputs_SetVoltage(initial_reference_v))
    {
        Outputs_SetSafe();
        return false;
    }

    /* Select CV mode and begin settling period */
    Outputs_SelectCV();
    cv_switch_start_ms = Timers_GetMillis();


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

    /*
     * Clear any previous band stop reason. The voltage band
     * itself starts when CV_STARTUP reaches the target.
     */
    UserLimits_Start(target_v, cv_last_update_ms);

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