#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Return the current system time in milliseconds.
 */
uint32_t Timers_GetMillis(void);

/*
 * Check whether a specified number of milliseconds
 * has elapsed since a previous time.
 */
bool Timers_HasElapsed(uint32_t start_ms, uint32_t period_ms);

#endif /* TIMERS_H */