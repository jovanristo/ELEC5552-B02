#include "protection.h"

#include <math.h>

BatteryTempResult protection_check_external_temperature(
    const float temperature_c[EXTERNAL_THERMISTOR_COUNT],
    const bool valid[EXTERNAL_THERMISTOR_COUNT],
    float cutoff_c)
{
    if (temperature_c == 0 || valid == 0 || !isfinite(cutoff_c)) {
        return (BatteryTempResult){BATTERY_TEMP_INVALID_INPUT, 0u};
    }

    for (unsigned i = 0; i < EXTERNAL_THERMISTOR_COUNT; ++i) {
        if (!valid[i] || !isfinite(temperature_c[i])) {
            return (BatteryTempResult){BATTERY_TEMP_INVALID_PROBE, i};
        }
        if (temperature_c[i] >= cutoff_c) {
            return (BatteryTempResult){BATTERY_TEMP_OVER_LIMIT, i};
        }
    }

    return (BatteryTempResult){BATTERY_TEMP_OK, 0u};
}

bool protection_external_temperature_is_ok(BatteryTempResult result)
{
    return result.fault == BATTERY_TEMP_OK;
}

InternalTempResult protection_check_internal_temperature(
    const float temperature_c[INTERNAL_THERMISTOR_COUNT],
    const bool valid[INTERNAL_THERMISTOR_COUNT])
{
    if (temperature_c == 0 || valid == 0) {
        return (InternalTempResult){INTERNAL_TEMP_INVALID_INPUT, 0u};
    }

    for (unsigned i = 0; i < INTERNAL_THERMISTOR_COUNT; ++i) {
        if (!valid[i] || !isfinite(temperature_c[i])) {
            return (InternalTempResult){INTERNAL_TEMP_INVALID_PROBE, i};
        }
        if (temperature_c[i] >= INTERNAL_HEATSINK_CUTOFF_C) {
            return (InternalTempResult){INTERNAL_TEMP_OVER_LIMIT, i};
        }
    }

    return (InternalTempResult){INTERNAL_TEMP_OK, 0u};
}

bool protection_internal_temperature_is_ok(InternalTempResult result)
{
    return result.fault == INTERNAL_TEMP_OK;
}

CurrentResult protection_check_current(
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT])
{
    if (current_a == 0) {
        return (CurrentResult){CURRENT_INVALID_INPUT, 0u};
    }

    for (unsigned i = 0; i < PROTECTION_CURRENT_CHANNEL_COUNT; ++i) {
        if (!isfinite(current_a[i]) || current_a[i] < 0.0f) {
            return (CurrentResult){CURRENT_INVALID_CHANNEL, i};
        }
        if (current_a[i] >= PROTECTION_CURRENT_LIMIT_A) {
            return (CurrentResult){CURRENT_OVER_LIMIT, i};
        }
    }

    return (CurrentResult){CURRENT_OK, 0u};
}

bool protection_current_is_ok(CurrentResult result)
{
    return result.fault == CURRENT_OK;
}

VoltageFault protection_check_voltage(float voltage_v)
{
    if (!isfinite(voltage_v) || voltage_v < 0.0f) {
        return VOLTAGE_INVALID_READING;
    }
    if (voltage_v >= PROTECTION_VOLTAGE_LIMIT_V) {
        return VOLTAGE_OVER_LIMIT;
    }
    return VOLTAGE_OK;
}

bool protection_voltage_is_ok(VoltageFault result)
{
    return result == VOLTAGE_OK;
}

bool protection_calculate_power(
    float voltage_v,
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT],
    float *power_w)
{
    if (power_w == 0) {
        return false;
    }
    *power_w = NAN;
    if (!isfinite(voltage_v) || voltage_v < 0.0f || current_a == 0) {
        return false;
    }

    float total_current_a = 0.0f;
    for (unsigned i = 0; i < PROTECTION_CURRENT_CHANNEL_COUNT; ++i) {
        if (!isfinite(current_a[i]) || current_a[i] < 0.0f) {
            return false;
        }
        total_current_a += current_a[i];
    }
    const float calculated_w = voltage_v * total_current_a;
    if (!isfinite(calculated_w)) {
        return false;
    }
    *power_w = calculated_w;
    return true;
}

TimeFault protection_check_time(float elapsed_s, float duration_s)
{
    if (!isfinite(elapsed_s) || elapsed_s < 0.0f ||
        !isfinite(duration_s) || duration_s <= 0.0f) {
        return TIME_INVALID_INPUT;
    }
    if (elapsed_s >= duration_s) {
        return TIME_LIMIT_REACHED;
    }
    return TIME_OK;
}

bool protection_time_is_ok(TimeFault result)
{
    return result == TIME_OK;
}
