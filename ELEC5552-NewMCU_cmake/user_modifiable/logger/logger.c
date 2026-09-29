#include "logger.h"

#include "inputs.h"
#include "storage.h"
#include "usb.h"
#include "timers.h"

#include <stdio.h>


static bool running = false;

static uint32_t start_time_ms = 0U;
static uint32_t last_sample_ms = 0U;

static uint32_t run_time_ms = 0U;
static uint32_t sample_period_ms = 100U;


/* ---------------------------------------------------------
 * Start a logging run
 * --------------------------------------------------------- */

bool Logger_Start(uint32_t requested_run_time_ms,
                  uint32_t requested_sample_period_ms)
{
    if ((requested_run_time_ms == 0U) ||
        (requested_sample_period_ms == 0U))
    {
        return false;
    }

    run_time_ms = requested_run_time_ms;

    /*
     * If USB is connected, use the user's requested
     * sample period because samples can be streamed.
     */
    if (USB_IsConnected())
    {
        sample_period_ms = requested_sample_period_ms;
    }

    /*
     * If USB is not connected, make sure the entire
     * run will fit in local storage.
     */
    else
    {
        uint32_t capacity = Storage_GetCapacity();

        if (capacity == 0U)
        {
            return false;
        }

        uint32_t minimum_period_ms =
            (run_time_ms + capacity - 1U) / capacity;

        if (requested_sample_period_ms < minimum_period_ms)
        {
            sample_period_ms = minimum_period_ms;
        }
        else
        {
            sample_period_ms = requested_sample_period_ms;
        }

        Storage_Clear();
    }

    start_time_ms = Timers_GetMillis();
    last_sample_ms = start_time_ms;

    running = true;

    return true;
}


/* ---------------------------------------------------------
 * Main logger update
 * --------------------------------------------------------- */

void Logger_Update(void)
{
    if (!running)
    {
        return;
    }

    uint32_t now = Timers_GetMillis();

    /* Stop when run duration is complete */
    if ((uint32_t)(now - start_time_ms) >= run_time_ms)
    {
        Logger_Stop();
        return;
    }

    /* Wait until the next sample time */
    if ((uint32_t)(now - last_sample_ms) < sample_period_ms)
    {
        return;
    }

    last_sample_ms = now;


    const Inputs_t *inputs = Inputs_Get();

    if (inputs == NULL)
    {
        return;
    }


    LogSample_t sample;

    sample.time_ms = now - start_time_ms;

    sample.dut_voltage_v = inputs->dut_voltage_v;

    sample.current_side11_a = inputs->current_side11_a;
    sample.current_side12_a = inputs->current_side12_a;
    sample.current_side21_a = inputs->current_side21_a;
    sample.current_side22_a = inputs->current_side22_a;

    for (uint32_t i = 0U; i < 12U; i++)
    {
        sample.temperature_c[i] =
            inputs->thermistor_temp_c[i];
    }


    /*
     * USB connected:
     * stream sample to computer.
     */
    if (USB_IsConnected())
    {
        char line[160];

        int length = snprintf(
            line,
            sizeof(line),
            "%lu,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",
            (unsigned long)sample.time_ms,
            sample.dut_voltage_v,
            sample.current_side11_a,
            sample.current_side12_a,
            sample.current_side21_a,
            sample.current_side22_a
        );

        if (length > 0)
        {
            USB_Send(
                (const uint8_t *)line,
                (uint32_t)length
            );
        }
    }

    /*
     * No USB:
     * save locally.
     */
    else
    {
        Storage_Write(&sample);
    }
}


/* ---------------------------------------------------------
 * Stop logging
 * --------------------------------------------------------- */

void Logger_Stop(void)
{
    running = false;
}


/* ---------------------------------------------------------
 * Get current logger state
 * --------------------------------------------------------- */

bool Logger_IsRunning(void)
{
    return running;
}


/* ---------------------------------------------------------
 * Return actual logging period being used
 * --------------------------------------------------------- */

uint32_t Logger_GetSamplePeriod(void)
{
    return sample_period_ms;
}