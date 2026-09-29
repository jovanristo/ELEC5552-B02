#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdint.h>

bool Logger_Start(uint32_t run_time_ms,
                  uint32_t requested_sample_period_ms);

void Logger_Update(void);

void Logger_Stop(void);

bool Logger_IsRunning(void);

uint32_t Logger_GetSamplePeriod(void);

#endif