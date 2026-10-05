#include "storage.h"

#include <stddef.h>


/* ---------------------------------------------------------
 * Temporary RAM storage
 * --------------------------------------------------------- */

static LogSample_t samples[STORAGE_MAX_SAMPLES];

static uint32_t sample_count = 0U;


/* ---------------------------------------------------------
 * Clear storage
 * --------------------------------------------------------- */

void Storage_Clear(void)
{
    sample_count = 0U;
}


/* ---------------------------------------------------------
 * Store one sample
 * --------------------------------------------------------- */

bool Storage_Write(const LogSample_t *sample)
{
    if (sample == NULL)
    {
        return false;
    }


    if (sample_count >= STORAGE_MAX_SAMPLES)
    {
        return false;
    }


    samples[sample_count] = *sample;

    sample_count++;

    return true;
}


/* ---------------------------------------------------------
 * Get number of stored samples
 * --------------------------------------------------------- */

uint32_t Storage_GetCount(void)
{
    return sample_count;
}


/* ---------------------------------------------------------
 * Get total storage capacity
 * --------------------------------------------------------- */

uint32_t Storage_GetCapacity(void)
{
    return STORAGE_MAX_SAMPLES;
}


/* ---------------------------------------------------------
 * Get remaining storage capacity
 * --------------------------------------------------------- */

uint32_t Storage_GetFreeCount(void)
{
    return STORAGE_MAX_SAMPLES - sample_count;
}


/* ---------------------------------------------------------
 * Check whether storage is full
 * --------------------------------------------------------- */

bool Storage_IsFull(void)
{
    return (sample_count >= STORAGE_MAX_SAMPLES);
}


/* ---------------------------------------------------------
 * Retrieve a stored sample
 * --------------------------------------------------------- */

const LogSample_t *Storage_GetSample(uint32_t index)
{
    if (index >= sample_count)
    {
        return NULL;
    }

    return &samples[index];
}