#ifndef PROTECTION_H
#define PROTECTION_H

#include <stdbool.h>
#include <stdint.h>


/* =========================================================
 * Thermistor configuration
 * ========================================================= */

/*
 * Total:
 *
 * 8 internal thermistors
 * 4 external DUT thermistors
 *
 * External thermistors are physical thermistors:
 * 4, 5, 6, 7
 *
 * which correspond to Inputs_t array indices:
 * 3, 4, 5, 6
 */

#define INTERNAL_THERMISTOR_COUNT        8U
#define EXTERNAL_THERMISTOR_COUNT        4U


/* =========================================================
 * Electrical protection configuration
 * ========================================================= */

#define PROTECTION_CURRENT_CHANNEL_COUNT 4U


/*
 * Internal heatsink temperature protection.
 *
 * Provisional value.
 * Confirm during thermal testing.
 */
#define INTERNAL_HEATSINK_CUTOFF_C       80.0f


/*
 * Total current protection.
 *
 * Normal device operating maximum = 30 A.
 * Protection trip = 32 A.
 */
#define PROTECTION_TOTAL_CURRENT_LIMIT_A 32.0f


/*
 * DUT voltage protection.
 *
 * Normal operating maximum = 30 V.
 * Protection trip = 32 V.
 */
#define PROTECTION_VOLTAGE_LIMIT_V       32.0f

#define PROTECTION_POWER_LIMIT_W         520.0f

#define PROTECTION_BATTERY_TEMP_LIMIT_C  60.0f


/* =========================================================
 * External temperature faults
 * ========================================================= */

typedef enum
{
    BATTERY_TEMP_OK = 0,

    BATTERY_TEMP_INVALID_PROBE,

    BATTERY_TEMP_OVER_LIMIT,

    BATTERY_TEMP_INVALID_INPUT

} BatteryTempFault;


typedef struct
{
    BatteryTempFault fault;

    /*
     * Index within the group of four external probes:
     *
     * 0 -> physical thermistor 4
     * 1 -> physical thermistor 5
     * 2 -> physical thermistor 6
     * 3 -> physical thermistor 7
     */
    uint8_t probe_index;

} BatteryTempResult;


/* =========================================================
 * Internal temperature faults
 * ========================================================= */

typedef enum
{
    INTERNAL_TEMP_OK = 0,

    INTERNAL_TEMP_INVALID_PROBE,

    INTERNAL_TEMP_OVER_LIMIT,

    INTERNAL_TEMP_INVALID_INPUT

} InternalTempFault;


typedef struct
{
    InternalTempFault fault;

    /*
     * Index within the group of eight internal probes.
     */
    uint8_t probe_index;

} InternalTempResult;


/* =========================================================
 * Current faults
 * ========================================================= */

typedef enum
{
    CURRENT_OK = 0,

    CURRENT_INVALID_CHANNEL,

    CURRENT_OVER_LIMIT,

    CURRENT_INVALID_INPUT

} CurrentFault;


typedef struct
{
    CurrentFault fault;

    /*
     * Current measurement channel that caused an error,
     * when applicable.
     */
    uint8_t channel_index;


    /*
     * Sum of all four measured branch currents.
     */
    float total_current_a;

} CurrentResult;


/* =========================================================
 * Voltage faults
 * ========================================================= */

typedef enum
{
    VOLTAGE_OK = 0,

    VOLTAGE_INVALID_READING,

    VOLTAGE_OVER_LIMIT,

    VOLTAGE_UNDER_LIMIT

} VoltageFault;


/* =========================================================
 * Overall protection fault
 * ========================================================= */

typedef enum
{
    PROTECTION_OK = 0,

    PROTECTION_INTERNAL_TEMP_FAULT,

    PROTECTION_EXTERNAL_TEMP_FAULT,

    PROTECTION_CURRENT_FAULT,

    PROTECTION_VOLTAGE_FAULT,

    PROTECTION_POWER_FAULT

} ProtectionFault;


/* =========================================================
 * Complete protection status
 * ========================================================= */

typedef struct
{
    ProtectionFault fault;


    InternalTempResult internal_temp;

    BatteryTempResult external_temp;

    CurrentResult current;

    VoltageFault voltage;


    /*
     * Current measured DUT power.
     *
     * This is calculated for monitoring.

     */
    float total_power_w;


    /*
     * Once a fault occurs this stays TRUE until
     * Protection_Reset() is called.
     */
    bool fault_latched;

} ProtectionStatus_t;


/* =========================================================
 * Main protection interface
 * ========================================================= */

void Protection_Init(void);


/*
 * Run all protection checks using Inputs_Get()
 * and the user limits in App_GetState().
 *
 * test_running enables the lower voltage check.
 */
void Protection_Update(bool test_running);


/*
 * Returns false if any fault has been latched.
 */
bool Protection_IsOK(void);


/*
 * Clear protection latch.
 *
 * This does NOT turn the electronic load back on.
 */
void Protection_Reset(void);


/*
 * Access detailed fault information.
 */
const ProtectionStatus_t *Protection_GetStatus(void);


/* =========================================================
 * Individual protection checks
 *
 * Mainly useful for testing/debugging.
 * ========================================================= */

BatteryTempResult Protection_CheckExternalTemperature(
    const float temperature_c[EXTERNAL_THERMISTOR_COUNT],
    const bool valid[EXTERNAL_THERMISTOR_COUNT],
    float cutoff_c,
    uint8_t probe_count);


InternalTempResult Protection_CheckInternalTemperature(
    const float temperature_c[INTERNAL_THERMISTOR_COUNT],
    const bool valid[INTERNAL_THERMISTOR_COUNT]);


CurrentResult Protection_CheckCurrent(
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT],
    float limit_a);


VoltageFault Protection_CheckVoltage(
    float voltage_v,
    float min_v,
    float max_v,
    bool check_min);

bool Protection_CalculatePower(
    float voltage_v,
    const float current_a[PROTECTION_CURRENT_CHANNEL_COUNT],
    float *power_w);


#endif /* PROTECTION_H */