#ifndef STORAGE_H
#define STORAGE_H

#include <stdbool.h>
#include <stdint.h>

#include "inputs.h"


/*
 * Temporary RAM storage capacity.
 *
 * Change this later when real storage hardware is implemented.
 */
#define STORAGE_MAX_SAMPLES 256U


/*
 * One complete logged measurement.
 */
typedef struct
{
    uint32_t time_ms;

    float dut_voltage_v;
    float total_current_a;

    float current_side11_a;
    float current_side12_a;
    float current_side21_a;
    float current_side22_a;

    float temperature_c[NUM_THERMISTORS];

} LogSample_t;


/* Clear all stored samples */
void Storage_Clear(void);


/* Store one sample */
bool Storage_Write(const LogSample_t *sample);


/* Number of samples currently stored */
uint32_t Storage_GetCount(void);


/* Maximum number of samples that can be stored */
uint32_t Storage_GetCapacity(void);


/* Number of unused sample locations */
uint32_t Storage_GetFreeCount(void);


/* Check whether storage is full */
bool Storage_IsFull(void);


/*
 * Get a stored sample.
 *
 * Returns NULL if index is invalid.
 */
const LogSample_t *Storage_GetSample(uint32_t index);


#endif /* STORAGE_H */