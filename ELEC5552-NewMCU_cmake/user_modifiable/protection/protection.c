#include "protection.h"

#include "inputs.h"
#include "outputs.h"
#include "app.h"
#include <math.h>
#include <stddef.h>


/* =========================================================
 * Thermistor mapping
 *
 * Inputs_t uses zero-based indices:
 *
 * Thermistor 1  -> 0  internal
 * Thermistor 2  -> 1  internal
 * Thermistor 3  -> 2  internal
 *
 * Thermistor 4  -> 3  EXTERNAL
 * Thermistor 5  -> 4  EXTERNAL
 * Thermistor 6  -> 5  EXTERNAL
 * Thermistor 7  -> 6  EXTERNAL
 *
 * Thermistor 8  -> 7  internal
 * Thermistor 9  -> 8  internal
 * Thermistor 10 -> 9  internal
 * Thermistor 11 -> 10 internal
 * Thermistor 12 -> 11 internal
 * ========================================================= */

static const uint8_t internal_thermistor_indices
    [INTERNAL_THERMISTOR_COUNT] =
{
    0U,
    1U,
    2U,
    7U,
    8U,
    9U,
    10U,
    11U
};


static const uint8_t external_thermistor_indices
    [EXTERNAL_THERMISTOR_COUNT] =
{
    3U,
    4U,
    5U,
    6U
};


/* =========================================================
 * Current protection state
 * ========================================================= */

static ProtectionStatus_t protection_status;


/* =========================================================
 * External DUT temperature protection
 * ========================================================= */

BatteryTempResult Protection_CheckExternalTemperature(
    const float temperature_c[EXTERNAL_THERMISTOR_COUNT],
    const bool valid[EXTERNAL_THERMISTOR_COUNT],
    float cutoff_c,
    uint8_t probe_count)
{
    uint8_t working_count = 0U;


    if ((temperature_c == NULL) ||
        (valid == NULL) ||
        (!isfinite(cutoff_c)))
    {
        return (BatteryTempResult)
        {
            .fault = BATTERY_TEMP_INVALID_INPUT,
            .probe_index = 0U
        };
    }


    if (probe_count > EXTERNAL_THERMISTOR_COUNT)
    {
        probe_count = EXTERNAL_THERMISTOR_COUNT;
    }


    /*
     * 0 probes selected = no battery temperature checking
     * (e.g. testing a power supply).
     */
    if (probe_count == 0U)
    {
        return (BatteryTempResult)
        {
            .fault = BATTERY_TEMP_OK,
            .probe_index = 0U
        };
    }


    /*
     * Check every working probe.
     * Broken probes are skipped, not treated as a fault.
     */
    for (uint8_t i = 0U;
         i < EXTERNAL_THERMISTOR_COUNT;
         i++)
    {
        if ((!valid[i]) ||
            (!isfinite(temperature_c[i])))
        {
            continue;
        }


        working_count++;


        if (temperature_c[i] >= cutoff_c)
        {
            return (BatteryTempResult)
            {
                .fault = BATTERY_TEMP_OVER_LIMIT,
                .probe_index = i
            };
        }
    }


    /*
     * User selected more probes than are working.
     */
    if (working_count < probe_count)
    {
        return (BatteryTempResult)
        {
            .fault = BATTERY_TEMP_INVALID_PROBE,
            .probe_index = working_count
        };
    }


    return (BatteryTempResult)
    {
        .fault = BATTERY_TEMP_OK,
        .probe_index = 0U
    };
}

/* =========================================================
 * Internal heatsink / MOSFET temperature protection
 * ========================================================= */

InternalTempResult Protection_CheckInternalTemperature(
    const float temperature_c[INTERNAL_THERMISTOR_COUNT],
    const bool valid[INTERNAL_THERMISTOR_COUNT])
{
    if ((temperature_c == NULL) ||
        (valid == NULL))
    {
        return (InternalTempResult)
        {
            .fault = INTERNAL_TEMP_INVALID_INPUT,
            .probe_index = 0U
        };
    }


    for (uint8_t i = 0U;
         i < INTERNAL_THERMISTOR_COUNT;
         i++)
    {
        if ((!valid[i]) ||
            (!isfinite(temperature_c[i])))
        {
            return (InternalTempResult)
            {
                .fault = INTERNAL_TEMP_INVALID_PROBE,
                .probe_index = i
            };
        }


        if (temperature_c[i] >=
            INTERNAL_HEATSINK_CUTOFF_C)
        {
            return (InternalTempResult)
            {
                .fault = INTERNAL_TEMP_OVER_LIMIT,
                .probe_index = i
            };
        }
    }


    return (InternalTempResult)
    {
        .fault = INTERNAL_TEMP_OK,
        .probe_index = 0U
    };
}


/* =========================================================
 * Total current protection
 * ========================================================= */

CurrentResult Protection_CheckCurrent(
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT],
    float limit_a)
{
    float total_current_a = 0.0f;


    if (current_a == NULL)
    {
        return (CurrentResult)
        {
            .fault = CURRENT_INVALID_INPUT,
            .channel_index = 0U,
            .total_current_a = 0.0f
        };
    }


    for (uint8_t i = 0U;
         i < PROTECTION_CURRENT_CHANNEL_COUNT;
         i++)
    {
        if ((!isfinite(current_a[i])) ||
            (current_a[i] < 0.0f))
        {
            return (CurrentResult)
            {
                .fault = CURRENT_INVALID_CHANNEL,
                .channel_index = i,
                .total_current_a = total_current_a
            };
        }


        total_current_a += current_a[i];
    }


    if (total_current_a >= limit_a)
    {
        return (CurrentResult)
        {
            .fault = CURRENT_OVER_LIMIT,
            .channel_index = 0U,
            .total_current_a = total_current_a
        };
    }


    return (CurrentResult)
    {
        .fault = CURRENT_OK,
        .channel_index = 0U,
        .total_current_a = total_current_a
    };
}
/* =========================================================
 * DUT voltage protection
 * ========================================================= */
VoltageFault Protection_CheckVoltage(
    float voltage_v,
    float min_v,
    float max_v,
    bool check_min)
{
    if ((!isfinite(voltage_v)) ||
        (voltage_v < 0.0f))
    {
        return VOLTAGE_INVALID_READING;
    }


    if (voltage_v >= max_v)
    {
        return VOLTAGE_OVER_LIMIT;
    }


    /*
     * Lower limit only applies while a test is running.
     * Otherwise 0 V at idle would trip it.
     */
    if (check_min && (voltage_v < min_v))
    {
        return VOLTAGE_UNDER_LIMIT;
    }


    return VOLTAGE_OK;
}

/* =========================================================
 * DUT power calculation
 *
 * P = V x I_total
 * ========================================================= */

bool Protection_CalculatePower(
    float voltage_v,
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT],
    float *power_w)
{
    float total_current_a = 0.0f;


    if ((current_a == NULL) ||
        (power_w == NULL))
    {
        return false;
    }


    if ((!isfinite(voltage_v)) ||
        (voltage_v < 0.0f))
    {
        return false;
    }


    for (uint8_t i = 0U;
         i < PROTECTION_CURRENT_CHANNEL_COUNT;
         i++)
    {
        if ((!isfinite(current_a[i])) ||
            (current_a[i] < 0.0f))
        {
            return false;
        }


        total_current_a +=
            current_a[i];
    }


    *power_w =
        voltage_v * total_current_a;


    return true;
}


/* =========================================================
 * Initialise protection system
 * ========================================================= */

void Protection_Init(void)
{
    protection_status.fault =
        PROTECTION_OK;


    protection_status.internal_temp.fault =
        INTERNAL_TEMP_OK;

    protection_status.internal_temp.probe_index =
        0U;


    protection_status.external_temp.fault =
        BATTERY_TEMP_OK;

    protection_status.external_temp.probe_index =
        0U;


    protection_status.current.fault =
        CURRENT_OK;

    protection_status.current.channel_index =
        0U;

    protection_status.current.total_current_a =
        0.0f;


    protection_status.voltage =
        VOLTAGE_OK;


    protection_status.total_power_w =
        0.0f;


    protection_status.fault_latched =
        false;
}


/* =========================================================
 * Run complete protection system
 * ========================================================= */
void Protection_Update(bool test_running)
{
    const Inputs_t *inputs =
        Inputs_Get();

    const AppState_t *app =
        App_GetState();


    if ((inputs == NULL) ||
        (app == NULL))
    {
        /*
         * Measurements or settings unavailable.
         *
         * Fail safe.
         */
        protection_status.fault =
            PROTECTION_CURRENT_FAULT;

        protection_status.fault_latched =
            true;


        Outputs_SetSafe();


        return;
    }


    /* =====================================================
     * Effective limits
     *
     * Whichever is lower: user limit or hard limit.
     * ===================================================== */

    float voltage_max_v =
        fminf(app->user_voltage_max_v,
              PROTECTION_VOLTAGE_LIMIT_V);

    float current_max_a =
        fminf(app->user_current_max_a,
              PROTECTION_TOTAL_CURRENT_LIMIT_A);

    float power_max_w =
        fminf(app->user_power_max_w,
              PROTECTION_POWER_LIMIT_W);

    float battery_temp_max_c =
        fminf(app->user_battery_temp_max_c,
              PROTECTION_BATTERY_TEMP_LIMIT_C);

    float voltage_min_v =
        app->user_voltage_min_v;


    /* =====================================================
     * Gather internal thermistors
     * ===================================================== */

    float internal_temp[INTERNAL_THERMISTOR_COUNT];

    bool internal_valid[INTERNAL_THERMISTOR_COUNT];


    for (uint8_t i = 0U;
         i < INTERNAL_THERMISTOR_COUNT;
         i++)
    {
        uint8_t input_index =
            internal_thermistor_indices[i];


        internal_temp[i] =
            inputs->thermistor_temp_c[input_index];


        internal_valid[i] =
            inputs->thermistor_valid[input_index];
    }


    /* =====================================================
     * Gather external DUT thermistors
     * ===================================================== */

    float external_temp[EXTERNAL_THERMISTOR_COUNT];

    bool external_valid[EXTERNAL_THERMISTOR_COUNT];


    for (uint8_t i = 0U;
         i < EXTERNAL_THERMISTOR_COUNT;
         i++)
    {
        uint8_t input_index =
            external_thermistor_indices[i];


        external_temp[i] =
            inputs->thermistor_temp_c[input_index];


        external_valid[i] =
            inputs->thermistor_valid[input_index];
    }


    /* =====================================================
     * Gather current measurements
     * ===================================================== */

    float current_a[PROTECTION_CURRENT_CHANNEL_COUNT] =
    {
        inputs->current_side11_a,
        inputs->current_side12_a,
        inputs->current_side21_a,
        inputs->current_side22_a
    };


    /* =====================================================
     * Run temperature checks
     * ===================================================== */

    protection_status.internal_temp =
        Protection_CheckInternalTemperature(
            internal_temp,
            internal_valid);


     /*
     * Battery probes are only checked while a test is running.
     */
    if (test_running)
    {
        protection_status.external_temp =
            Protection_CheckExternalTemperature(
                external_temp,
                external_valid,
                battery_temp_max_c,
                app->user_probe_count);
    }
    else
    {
        protection_status.external_temp.fault =
            BATTERY_TEMP_OK;

        protection_status.external_temp.probe_index =
            0U;
    }

    /* =====================================================
     * Run electrical checks
     * ===================================================== */

    protection_status.current =
        Protection_CheckCurrent(
            current_a,
            current_max_a);


    protection_status.voltage =
        Protection_CheckVoltage(
            inputs->dut_voltage_v,
            voltage_min_v,
            voltage_max_v,
            test_running);


    /* =====================================================
     * Calculate and check DUT power
     * ===================================================== */

    bool power_over_limit = false;

    if (!Protection_CalculatePower(
            inputs->dut_voltage_v,
            current_a,
            &protection_status.total_power_w))
    {
        protection_status.total_power_w =
            0.0f;
    }
    else if (protection_status.total_power_w >=
             power_max_w)
    {
        power_over_limit = true;
    }


    /* =====================================================
     * Determine current fault
     *
     * Priority:
     *
     * 1. Internal temperature
     * 2. External DUT temperature
     * 3. Over-current
     * 4. Voltage (over or under)
     * 5. Over-power
     * ===================================================== */

    if (protection_status.internal_temp.fault !=
        INTERNAL_TEMP_OK)
    {
        protection_status.fault =
            PROTECTION_INTERNAL_TEMP_FAULT;
    }

    else if (protection_status.external_temp.fault !=
             BATTERY_TEMP_OK)
    {
        protection_status.fault =
            PROTECTION_EXTERNAL_TEMP_FAULT;
    }

    else if (protection_status.current.fault !=
             CURRENT_OK)
    {
        protection_status.fault =
            PROTECTION_CURRENT_FAULT;
    }

    else if (protection_status.voltage !=
             VOLTAGE_OK)
    {
        protection_status.fault =
            PROTECTION_VOLTAGE_FAULT;
    }

    else if (power_over_limit)
    {
        protection_status.fault =
            PROTECTION_POWER_FAULT;
    }

    else
    {
        protection_status.fault =
            PROTECTION_OK;
    }


    /* =====================================================
     * Latch fault and force electronic load safe
     * ===================================================== */

    if (protection_status.fault !=
        PROTECTION_OK)
    {
        protection_status.fault_latched =
            true;


        /*
         * DAC -> 0 V.
         *
         * Fan is intentionally NOT stopped here.
         */
        Outputs_SetSafe();
    }
}

/* =========================================================
 * Protection state
 * ========================================================= */

bool Protection_IsOK(void)
{
    return !protection_status.fault_latched;
}


/* =========================================================
 * Reset latched fault
 * ========================================================= */

void Protection_Reset(void)
{
    /*
     * Resetting protection does not command any load.
     *
     * The control system still has to start a new test.
     */

    protection_status.fault =
        PROTECTION_OK;


    protection_status.internal_temp.fault =
        INTERNAL_TEMP_OK;

    protection_status.internal_temp.probe_index =
        0U;


    protection_status.external_temp.fault =
        BATTERY_TEMP_OK;

    protection_status.external_temp.probe_index =
        0U;


    protection_status.current.fault =
        CURRENT_OK;

    protection_status.current.channel_index =
        0U;

    protection_status.current.total_current_a =
        0.0f;


    protection_status.voltage =
        VOLTAGE_OK;


    protection_status.total_power_w =
        0.0f;


    protection_status.fault_latched =
        false;
}


/* =========================================================
 * Get detailed protection state
 * ========================================================= */

const ProtectionStatus_t *Protection_GetStatus(void)
{
    return &protection_status;
}