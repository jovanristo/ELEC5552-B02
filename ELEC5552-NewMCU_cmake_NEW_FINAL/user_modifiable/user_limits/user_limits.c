#include "user_limits.h"

#include <math.h>


/* Setpoint change smaller than this is not treated as a change */
#define USER_LIMIT_TARGET_EPSILON   0.001f


static float    last_target;
static bool     armed;
static uint32_t settle_start_ms;
static bool     out_of_band;
static uint32_t out_of_band_since_ms;

static UserLimitResult_t last_result = USER_LIMIT_OK;


/* ---------------------------------------------------------
 * Disarm the band and restart the settle timer.
 *
 * Used at test start and whenever the setpoint changes,
 * because the reading needs time to move to the new band.
 * --------------------------------------------------------- */
static void UserLimits_Rearm(float target, uint32_t now_ms)
{
    last_target     = target;
    armed           = false;
    settle_start_ms = now_ms;
    out_of_band     = false;
}


void UserLimits_Start(float target, uint32_t now_ms)
{
    UserLimits_Rearm(target, now_ms);

    last_result = USER_LIMIT_OK;
}


UserLimitResult_t UserLimits_Update(float target,
                                    float centre,
                                    float band,
                                    float measured,
                                    uint32_t now_ms)
{
    if (!isfinite(target)  ||
        !isfinite(centre)  ||
        !isfinite(measured))
    {
        /* Invalid readings are handled by protection.c */
        return USER_LIMIT_OK;
    }


    /*
     * User changed the setpoint during the test:
     * move the band and wait for the reading to enter it.
     */
    if (fabsf(target - last_target) > USER_LIMIT_TARGET_EPSILON)
    {
        UserLimits_Rearm(target, now_ms);
    }


    /*
     * Band is recalculated every loop, so it follows the
     * setpoint (and, in CC mode, the 500 W power cap).
     */
    float lower = centre - band;
    float upper = centre + band;

    bool in_band = (measured >= lower) && (measured <= upper);


    /* ---------------- Not yet armed ---------------- */
    if (!armed)
    {
        if (in_band)
        {
            armed       = true;
            out_of_band = false;

            return USER_LIMIT_OK;
        }

        /* Unsigned subtraction handles tick wrap-around */
        if ((now_ms - settle_start_ms) >= USER_LIMIT_SETTLE_TIMEOUT_MS)
        {
            last_result = USER_LIMIT_NOT_SETTLED;
            return last_result;
        }

        return USER_LIMIT_OK;
    }


    /* ---------------- Armed ---------------- */
    if (in_band)
    {
        out_of_band = false;
        return USER_LIMIT_OK;
    }

    if (!out_of_band)
    {
        out_of_band          = true;
        out_of_band_since_ms = now_ms;

        return USER_LIMIT_OK;
    }

    if ((now_ms - out_of_band_since_ms) >= USER_LIMIT_DEBOUNCE_MS)
    {
        last_result = (measured < lower) ?
                      USER_LIMIT_BELOW_BAND :
                      USER_LIMIT_ABOVE_BAND;

        return last_result;
    }

    return USER_LIMIT_OK;
}


UserLimitResult_t UserLimits_GetLastResult(void)
{
    return last_result;
}
