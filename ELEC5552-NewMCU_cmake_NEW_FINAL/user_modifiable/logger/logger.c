#include "logger.h"

#include "inputs.h"
#include "storage.h"
#include "timers.h"
#include "usb.h"

#include <stdio.h>


/* ---------------------------------------------------------
 * Logger state
 * --------------------------------------------------------- */

static bool logger_running = false;

static uint32_t requested_sample_period_ms = 100U;
static uint32_t actual_sample_period_ms = 100U;

static uint32_t last_sample_ms = 0U;

static bool previous_usb_connected = false;


/* ---------------------------------------------------------
 * Calculate minimum sample period required to fit the
 * remaining run into the remaining local storage.
 * --------------------------------------------------------- */

static uint32_t Logger_CalculateStoragePeriod(void)
{
    uint32_t capacity =
        Storage_GetCapacity();

    uint32_t used =
        Storage_GetCount();

    if (capacity <= used)
    {
        return 0U;
    }

    uint32_t free_samples =
        capacity - used;

    uint32_t remaining_ms =
        Timers_RunTimerGetRemainingMs();

    if (remaining_ms == 0U)
    {
        return requested_sample_period_ms;
    }

    /*
     * Rounded-up division:
     *
     * remaining time / available samples
     */
    uint32_t minimum_period_ms =
        (remaining_ms + free_samples - 1U)
        / free_samples;

    /*
     * Never sample faster than the user requested.
     */
    if (minimum_period_ms <
        requested_sample_period_ms)
    {
        minimum_period_ms =
            requested_sample_period_ms;
    }

    return minimum_period_ms;
}


/* ---------------------------------------------------------
 * Start logger
 * --------------------------------------------------------- */

bool Logger_Start(uint32_t sample_period_ms)
{
    if (sample_period_ms == 0U)
    {
        return false;
    }

    if (Timers_RunTimerGetDurationMs() == 0U)
    {
        return false;
    }

    requested_sample_period_ms =
        sample_period_ms;

    previous_usb_connected =
        USB_IsConnected();


    /*
     * Start with empty local storage.
     *
     * Even if USB is initially connected, this means storage
     * is ready if USB disconnects during the run.
     */
    Storage_Clear();


    if (previous_usb_connected)
    {
        /*
         * Computer available:
         * use user's requested sampling rate.
         */
        actual_sample_period_ms =
            requested_sample_period_ms;
    }
    else
    {
        /*
         * No computer:
         * choose a period that guarantees the run fits.
         */
        actual_sample_period_ms =
            Logger_CalculateStoragePeriod();

        if (actual_sample_period_ms == 0U)
        {
            return false;
        }
    }


    if (!Timers_RunTimerStart())
    {
        return false;
    }


    last_sample_ms =
        Timers_GetMillis();

    logger_running = true;

    return true;
}


/* ---------------------------------------------------------
 * Main logger function
 * --------------------------------------------------------- */

void Logger_Update(void)
{
    if (!logger_running)
    {
        return;
    }


    /* -----------------------------------------------------
     * Stop when universal run timer finishes
     * ----------------------------------------------------- */

    if (Timers_RunTimerFinished())
    {
        Logger_Stop();

        return;
    }


    /* -----------------------------------------------------
     * Check USB connection state
     * ----------------------------------------------------- */

    bool usb_connected =
        USB_IsConnected();


    /*
     * USB has just disconnected.
     *
     * Recalculate a storage-safe logging period using:
     *
     * remaining run time
     * remaining storage capacity
     */
    if ((!usb_connected) &&
        previous_usb_connected)
    {
        uint32_t storage_period =
            Logger_CalculateStoragePeriod();

        if (storage_period != 0U)
        {
            actual_sample_period_ms =
                storage_period;
        }
    }


    /*
     * USB has just reconnected.
     *
     * We can return to the user's requested period.
     */
    if (usb_connected &&
        (!previous_usb_connected))
    {
        actual_sample_period_ms =
            requested_sample_period_ms;
    }


    previous_usb_connected =
        usb_connected;


    /* -----------------------------------------------------
     * Wait for next sample
     * ----------------------------------------------------- */

    uint32_t now =
        Timers_GetMillis();


    if ((uint32_t)(now - last_sample_ms)
        < actual_sample_period_ms)
    {
        return;
    }


    last_sample_ms = now;


    /* -----------------------------------------------------
     * Get latest measurements
     * ----------------------------------------------------- */

    const Inputs_t *inputs =
        Inputs_Get();


    if (inputs == NULL)
    {
        return;
    }


    /* -----------------------------------------------------
     * Create measurement record
     * ----------------------------------------------------- */

    LogSample_t sample;


    sample.time_ms =
        Timers_RunTimerGetElapsedMs();


    sample.dut_voltage_v =
        inputs->dut_voltage_v;


    sample.total_current_a =
        inputs->total_current_a;


    sample.current_side11_a =
        inputs->current_side11_a;

    sample.current_side12_a =
        inputs->current_side12_a;

    sample.current_side21_a =
        inputs->current_side21_a;

    sample.current_side22_a =
        inputs->current_side22_a;


    for (uint32_t i = 0U;
         i < NUM_THERMISTORS;
         i++)
    {
        sample.temperature_c[i] =
            inputs->thermistor_temp_c[i];
    }


    /* -----------------------------------------------------
     * Computer connected
     *
     * Stream measurement over USB.
     * ----------------------------------------------------- */

    if (usb_connected)
    {
        /* Include all twelve temperature measurements in the CSV output.
         * Local RAM storage already contains these values.
         */
        char line[320];
        int length = snprintf(
            line, sizeof(line),
            "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f",
            (unsigned long)sample.time_ms,
            sample.dut_voltage_v,
            sample.total_current_a,
            sample.current_side11_a,
            sample.current_side12_a,
            sample.current_side21_a,
            sample.current_side22_a
        );

        if (length < 0 || (size_t)length >= sizeof(line))
        {
            return; /* Do not send a truncated CSV record. */
        }

        /* Append the eight internal and four external temperatures. */
        for (uint32_t i = 0U; i < NUM_THERMISTORS; i++)
        {
            int added = snprintf(
                line + length,
                sizeof(line) - (size_t)length,
                ",%.2f",
                sample.temperature_c[i]
            );

            if (added < 0 || (size_t)added >= sizeof(line) - (size_t)length)
            {
                return; /* Do not send a truncated CSV record. */
            }
            length += added;
        }

        if (sizeof(line) - (size_t)length < 3U)
        {
            return;
        }
        line[length++] = '\r';
        line[length++] = '\n';
        line[length] = '\0';

        /* Existing USB_Send() may return before a non-blocking transfer
         * completes; reliable delivery still needs separate handling.
         */
        USB_Send((const uint8_t *)line, (uint32_t)length);
    }


    /* -----------------------------------------------------
     * No computer
     *
     * Store measurement locally.
     * ----------------------------------------------------- */

    else
    {
        Storage_Write(&sample);
    }
}


/* ---------------------------------------------------------
 * Stop logger and universal run timer
 * --------------------------------------------------------- */

void Logger_Stop(void)
{
    logger_running = false;

    Timers_RunTimerStop();
}


/* ---------------------------------------------------------
 * Check logger state
 * --------------------------------------------------------- */

bool Logger_IsRunning(void)
{
    return logger_running;
}


/* ---------------------------------------------------------
 * User-requested sampling period
 * --------------------------------------------------------- */

uint32_t Logger_GetRequestedSamplePeriod(void)
{
    return requested_sample_period_ms;
}


/* ---------------------------------------------------------
 * Actual sampling period
 * --------------------------------------------------------- */

uint32_t Logger_GetSamplePeriod(void)
{
    return actual_sample_period_ms;
}