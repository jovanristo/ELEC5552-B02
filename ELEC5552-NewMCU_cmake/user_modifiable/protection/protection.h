#ifndef PROTECTION_H
#define PROTECTION_H

#include "external_thermistors.h"
#include "internal_thermistors.h"

/* Provisional heatsink surface limit, in degrees C. Confirm with thermal
 * design before enabling hardware; this is separate from MOSFET junction Tj.
 */
#define INTERNAL_HEATSINK_CUTOFF_C 100.0f

typedef enum {
    BATTERY_TEMP_OK,
    BATTERY_TEMP_INVALID_PROBE,
    BATTERY_TEMP_OVER_LIMIT,
    BATTERY_TEMP_INVALID_INPUT
} BatteryTempFault;

typedef struct {
    BatteryTempFault fault;
    unsigned probe_index; /* 0..3 for a probe fault; unused otherwise. */
} BatteryTempResult;

/* Call after each new set of four converted external probe readings.
 * The caller turns the load off when fault != BATTERY_TEMP_OK.
 * The cutoff is supplied by the caller; no default is assumed here.
 */
BatteryTempResult protection_check_external_temperature(
    const float temperature_c[EXTERNAL_THERMISTOR_COUNT],
    const bool valid[EXTERNAL_THERMISTOR_COUNT],
    float cutoff_c);
bool protection_external_temperature_is_ok(BatteryTempResult result);

typedef enum {
    INTERNAL_TEMP_OK,
    INTERNAL_TEMP_INVALID_PROBE,
    INTERNAL_TEMP_OVER_LIMIT,
    INTERNAL_TEMP_INVALID_INPUT
} InternalTempFault;

typedef struct {
    InternalTempFault fault;
    unsigned probe_index; /* 0..7 for MOSFET/heatsink probe; unused if OK. */
} InternalTempResult;

/* Check all eight heatsink probes after each fresh conversion. The caller
 * shuts off the load for any result other than INTERNAL_TEMP_OK.
 */
InternalTempResult protection_check_internal_temperature(
    const float temperature_c[INTERNAL_THERMISTOR_COUNT],
    const bool valid[INTERNAL_THERMISTOR_COUNT]);
bool protection_internal_temperature_is_ok(InternalTempResult result);

/* Provisional current protection for four measured channels. The ADC driver
 * must convert each pin reading to amperes before calling this function.
 * Confirm whether 4 A applies per channel or to their combined current.
 */
#define PROTECTION_CURRENT_CHANNEL_COUNT 4u
#define PROTECTION_CURRENT_LIMIT_A 32.0f

typedef enum {
    CURRENT_OK,
    CURRENT_INVALID_CHANNEL,
    CURRENT_OVER_LIMIT,
    CURRENT_INVALID_INPUT
} CurrentFault;

typedef struct {
    CurrentFault fault;
    unsigned channel_index; /* 0..3 for a channel fault; unused if OK. */
} CurrentResult;

CurrentResult protection_check_current(
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT]);
bool protection_current_is_ok(CurrentResult result);

/* Provisional input-voltage cutoff. Pass a converted voltage in volts. */
#define PROTECTION_VOLTAGE_LIMIT_V 32.0f

typedef enum {
    VOLTAGE_OK,
    VOLTAGE_INVALID_READING,
    VOLTAGE_OVER_LIMIT
} VoltageFault;

VoltageFault protection_check_voltage(float voltage_v);
bool protection_voltage_is_ok(VoltageFault result);

/* Sum the four independent branch currents and calculate total load power.
 * Returns false for missing, negative, or non-finite values. */
bool protection_calculate_power(
    float voltage_v,
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT],
    float *power_w);

typedef enum {
    TIME_OK,
    TIME_LIMIT_REACHED,
    TIME_INVALID_INPUT
} TimeFault;

/* Caller supplies elapsed test time and user-selected duration in seconds.
 * The caller stops the load when the limit is reached or input is invalid. */
TimeFault protection_check_time(float elapsed_s, float duration_s);
bool protection_time_is_ok(TimeFault result);

#endif /* PROTECTION_H */
