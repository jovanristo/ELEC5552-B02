#ifndef TIMERS_H
#define TIMERS_H

#include <stdbool.h>
#include <stdint.h>


typedef struct
{
    uint32_t hours;
    uint32_t minutes;
    uint32_t seconds;

} TimerHMS_t;


/* General timing functions */
uint32_t Timers_GetMillis(void);

bool Timers_HasElapsed(uint32_t start_ms,
                       uint32_t period_ms);


/* ---------------------------------------------------------
 * Universal run timer
 * --------------------------------------------------------- */

/* Set user-selected run duration */
void Timers_RunTimerSet(uint32_t hours,
                        uint32_t minutes,
                        uint32_t seconds);

/* Start / stop */
bool Timers_RunTimerStart(void);
void Timers_RunTimerStop(void);

/* Status */
bool Timers_RunTimerIsRunning(void);
bool Timers_RunTimerFinished(void);

/* Time values */
uint32_t Timers_RunTimerGetDurationMs(void);
uint32_t Timers_RunTimerGetElapsedMs(void);
uint32_t Timers_RunTimerGetRemainingMs(void);

/* Convenient values for UI */
TimerHMS_t Timers_RunTimerGetDurationHMS(void);
TimerHMS_t Timers_RunTimerGetElapsedHMS(void);
TimerHMS_t Timers_RunTimerGetRemainingHMS(void);


#endif /* TIMERS_H */