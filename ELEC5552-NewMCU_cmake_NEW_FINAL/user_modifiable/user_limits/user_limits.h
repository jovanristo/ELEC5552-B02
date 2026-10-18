#ifndef USER_LIMITS_H
#define USER_LIMITS_H

#include <stdbool.h>
#include <stdint.h>

/* ---------------------------------------------------------
 * User operating band (REQ-10)
 *
 * The test stops if the regulated quantity leaves a band
 * around its setpoint:
 *
 *   CC mode: total current  within setpoint +/- 1 A
 *   CV mode: DUT voltage    within setpoint +/- 1 V
 *
 * This is separate from protection.c, which only enforces
 * the fixed hard limits (32 V, 32 A, 520 W).
 * --------------------------------------------------------- */

#define USER_LIMIT_CURRENT_BAND_A      1.0f
#define USER_LIMIT_VOLTAGE_BAND_V      1.0f

/* Time allowed for the reading to first enter the band
 * after a start or a setpoint change. */
#define USER_LIMIT_SETTLE_TIMEOUT_MS   10000U

/* Reading must stay outside the band this long before the
 * test is stopped (rejects noise and brief transients). */
#define USER_LIMIT_DEBOUNCE_MS         200U


typedef enum
{
    USER_LIMIT_OK = 0,
    USER_LIMIT_NOT_SETTLED,     /* never entered band within timeout */
    USER_LIMIT_BELOW_BAND,
    USER_LIMIT_ABOVE_BAND
} UserLimitResult_t;


/* Call when a CC or CV test starts (or CC changes to CV). */
void UserLimits_Start(float target, uint32_t now_ms);

/*
 * Call every control loop while a test is running.
 *
 * target   : user setpoint (a change re-arms the band)
 * centre   : value the band is built around
 *            (CC: power-capped current; CV: voltage setpoint)
 * band     : half-width of the band
 * measured : measured total current or DUT voltage
 */
UserLimitResult_t UserLimits_Update(float target,
                                    float centre,
                                    float band,
                                    float measured,
                                    uint32_t now_ms);

/* Reason the last test was stopped by the band check
 * (for display). USER_LIMIT_OK if it was not. */
UserLimitResult_t UserLimits_GetLastResult(void);

#endif /* USER_LIMITS_H */
