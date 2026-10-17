#include "timers.h"

#include "stm32_hal.h"


/* ---------------------------------------------------------
 * Universal run timer state
 * --------------------------------------------------------- */

static uint32_t run_duration_ms = 0U;
static uint32_t run_start_ms = 0U;
static uint32_t run_final_elapsed_ms = 0U;


static bool run_timer_running = false;


/* ---------------------------------------------------------
 * Convert milliseconds to hours/minutes/seconds
 * --------------------------------------------------------- */

static TimerHMS_t Timers_ConvertMsToHMS(uint32_t time_ms)
{
    TimerHMS_t time;

    uint32_t total_seconds = time_ms / 1000U;

    time.hours =
        total_seconds / 3600U;

    time.minutes =
        (total_seconds % 3600U) / 60U;

    time.seconds =
        total_seconds % 60U;

    return time;
}


/* ---------------------------------------------------------
 * General millisecond timer
 * --------------------------------------------------------- */

uint32_t Timers_GetMillis(void)
{
    return HAL_GetTick();
}


/* ---------------------------------------------------------
 * Check whether period has elapsed
 * --------------------------------------------------------- */

bool Timers_HasElapsed(uint32_t start_ms,
                       uint32_t period_ms)
{
    return ((uint32_t)(HAL_GetTick() - start_ms)
            >= period_ms);
}


/* ---------------------------------------------------------
 * Set run duration from UI
 * --------------------------------------------------------- */

void Timers_RunTimerSet(uint32_t hours,
                        uint32_t minutes,
                        uint32_t seconds)
{
    uint32_t total_seconds =
        (hours * 3600U) +
        (minutes * 60U) +
        seconds;

    run_duration_ms =
        total_seconds * 1000U;
}


/* ---------------------------------------------------------
 * Start run timer
 * --------------------------------------------------------- */

bool Timers_RunTimerStart(void)
{
    if (run_duration_ms == 0U)
    {
        return false;
    }

    run_final_elapsed_ms = 0U;
    run_start_ms = Timers_GetMillis();

    run_timer_running = true;

    return true;
}


/* ---------------------------------------------------------
 * Stop run timer
 * --------------------------------------------------------- */

void Timers_RunTimerStop(void)
{
    if (run_timer_running)
    {
        run_final_elapsed_ms =
            Timers_GetMillis() - run_start_ms;

        run_timer_running = false;
    }
}


/* ---------------------------------------------------------
 * Check whether timer is running
 * --------------------------------------------------------- */

bool Timers_RunTimerIsRunning(void)
{
    return run_timer_running;
}


/* ---------------------------------------------------------
 * Check whether run duration has finished
 * --------------------------------------------------------- */

bool Timers_RunTimerFinished(void)
{
    if (!run_timer_running)
    {
        return false;
    }

    return (Timers_RunTimerGetElapsedMs()
            >= run_duration_ms);
}


/* ---------------------------------------------------------
 * Get selected run duration
 * --------------------------------------------------------- */

uint32_t Timers_RunTimerGetDurationMs(void)
{
    return run_duration_ms;
}


/* ---------------------------------------------------------
 * Get elapsed run time
 * --------------------------------------------------------- */

uint32_t Timers_RunTimerGetElapsedMs(void)
{
    if (!run_timer_running)
    {
        return run_final_elapsed_ms;
    }

    return Timers_GetMillis() - run_start_ms;
}


/* ---------------------------------------------------------
 * Get remaining run time
 * --------------------------------------------------------- */

uint32_t Timers_RunTimerGetRemainingMs(void)
{
    uint32_t elapsed_ms =
        Timers_RunTimerGetElapsedMs();

    if (elapsed_ms >= run_duration_ms)
    {
        return 0U;
    }

    return run_duration_ms - elapsed_ms;
}


/* ---------------------------------------------------------
 * Duration as HH:MM:SS
 * --------------------------------------------------------- */

TimerHMS_t Timers_RunTimerGetDurationHMS(void)
{
    return Timers_ConvertMsToHMS(
        run_duration_ms
    );
}


/* ---------------------------------------------------------
 * Elapsed time as HH:MM:SS
 * --------------------------------------------------------- */

TimerHMS_t Timers_RunTimerGetElapsedHMS(void)
{
    return Timers_ConvertMsToHMS(
        Timers_RunTimerGetElapsedMs()
    );
}


/* ---------------------------------------------------------
 * Remaining time as HH:MM:SS
 * --------------------------------------------------------- */

TimerHMS_t Timers_RunTimerGetRemainingHMS(void)
{
    return Timers_ConvertMsToHMS(
        Timers_RunTimerGetRemainingMs()
    );
}