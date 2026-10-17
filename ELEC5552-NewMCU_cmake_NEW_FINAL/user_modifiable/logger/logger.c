#include "logger.h"
#include "inputs.h"
#include "storage.h"
#include "timers.h"
#include "usb.h"
#include <stdio.h>
#include <string.h>

/* Local RAM stores at most 256 samples; rate adapts to the run duration.
 * The host must open the USB CDC serial port before the run finishes. */
static bool logger_running = false;
static uint32_t requested_sample_period_ms = 100U;
static uint32_t actual_sample_period_ms = 100U;
static uint32_t last_sample_ms = 0U;
static LoggerUploadStatus_t upload_status = LOGGER_UPLOAD_NONE;
static uint32_t upload_index = 0U;
static bool upload_header_sent = false;
static char upload_line[384];

static uint32_t Logger_StoragePeriod(void)
{
    uint32_t free_count = Storage_GetFreeCount();
    /* Reserve one slot for the final shutdown sample. */
    if (free_count <= 1U) return 0U;
    --free_count;
    uint32_t remaining_ms = Timers_RunTimerGetRemainingMs();
    uint32_t period = remaining_ms / free_count;
    if ((remaining_ms % free_count) != 0U) ++period;
    return (period < requested_sample_period_ms) ? requested_sample_period_ms : period;
}

static void Logger_RecordNow(void)
{
    const Inputs_t *in = Inputs_Get();
    if (in == NULL || Storage_IsFull()) return;
    LogSample_t s = {0};
    s.time_ms = Timers_RunTimerGetElapsedMs();
    s.dut_voltage_v = in->dut_voltage_v;
    s.total_current_a = in->total_current_a;
    s.current_side11_a = in->current_side11_a;
    s.current_side12_a = in->current_side12_a;
    s.current_side21_a = in->current_side21_a;
    s.current_side22_a = in->current_side22_a;
    for (uint32_t i = 0U; i < NUM_THERMISTORS; ++i)
        s.temperature_c[i] = in->thermistor_temp_c[i];
    Storage_Write(&s);
}

bool Logger_Start(uint32_t period_ms)
{
    if (logger_running || period_ms == 0U ||
        Timers_RunTimerGetDurationMs() == 0U) return false;
    requested_sample_period_ms = period_ms;
    Storage_Clear(); /* New run replaces previous session's RAM log. */
    upload_status = LOGGER_UPLOAD_NONE;
    upload_index = 0U;
    upload_header_sent = false;
    if (!Timers_RunTimerStart()) return false;
    actual_sample_period_ms = Logger_StoragePeriod();
    if (actual_sample_period_ms == 0U)
    {
        Timers_RunTimerStop();
        return false;
    }
    last_sample_ms = Timers_GetMillis();
    logger_running = true;
    Logger_RecordNow();
    return true;
}

void Logger_Stop(void)
{
    if (!logger_running) return;
    Logger_RecordNow();
    logger_running = false;
    Timers_RunTimerStop();
    upload_status = Storage_GetCount() ? LOGGER_UPLOAD_WAITING_USB : LOGGER_UPLOAD_NONE;
    upload_index = 0U;
    upload_header_sent = false;
}

/* USBX write_run is nonblocking: resend the SAME static buffer until it
 * completes; do not change its contents while transfer is in progress. */
static void Logger_UploadStep(void)
{
    if (!USB_IsConnected())
    {
        upload_status = LOGGER_UPLOAD_WAITING_USB;
        upload_header_sent = false;
        upload_index = 0U;
        return;
    }
    if (upload_status == LOGGER_UPLOAD_WAITING_USB)
    {
        upload_status = LOGGER_UPLOAD_SENDING;
        upload_index = 0U;
        upload_header_sent = false;
        upload_line[0] = '\0';
    }

    if (!upload_header_sent)
    {
        static const char header[] =
            "time_ms,voltage_V,total_A,pair1_A,pair2_A,pair3_A,pair4_A,"
            "T1_C,T2_C,T3_C,T4_C,T5_C,T6_C,T7_C,T8_C,T9_C,T10_C,T11_C,T12_C\r\n";
        if (USB_Send((const uint8_t *)header, sizeof(header) - 1U))
            upload_header_sent = true;
        return;
    }
    if (upload_index >= Storage_GetCount())
    {
        upload_status = LOGGER_UPLOAD_COMPLETE;
        return;
    }
    const LogSample_t *s = Storage_GetSample(upload_index);
    if (s == NULL) return;
    /* The current line is generated anew only after the last write completes. */
    if (upload_line[0] == '\0')
    {
        int n = snprintf(upload_line, sizeof(upload_line),
            "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,"
            "%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\r\n",
            (unsigned long)s->time_ms, s->dut_voltage_v, s->total_current_a,
            s->current_side11_a, s->current_side12_a,
            s->current_side21_a, s->current_side22_a,
            s->temperature_c[0], s->temperature_c[1], s->temperature_c[2],
            s->temperature_c[3], s->temperature_c[4], s->temperature_c[5],
            s->temperature_c[6], s->temperature_c[7], s->temperature_c[8],
            s->temperature_c[9], s->temperature_c[10], s->temperature_c[11]);
        if (n <= 0 || (uint32_t)n >= sizeof(upload_line)) return;
    }
    if (USB_Send((const uint8_t *)upload_line, (uint32_t)
                 (sizeof(upload_line) > 0U ? strlen(upload_line) : 0U)))
    {
        ++upload_index;
        upload_line[0] = '\0';
    }
}

void Logger_Update(void)
{
    if (logger_running)
    {
        if (Timers_RunTimerFinished()) { Logger_Stop(); return; }
        actual_sample_period_ms = Logger_StoragePeriod();
        uint32_t now = Timers_GetMillis();
        if (actual_sample_period_ms &&
            (uint32_t)(now - last_sample_ms) >= actual_sample_period_ms)
        {
            last_sample_ms = now;
            Logger_RecordNow();
        }
    }
    else if (upload_status == LOGGER_UPLOAD_WAITING_USB ||
             upload_status == LOGGER_UPLOAD_SENDING)
    {
        Logger_UploadStep();
    }
}

bool Logger_IsRunning(void) { return logger_running; }
uint32_t Logger_GetRequestedSamplePeriod(void) { return requested_sample_period_ms; }
uint32_t Logger_GetSamplePeriod(void) { return actual_sample_period_ms; }
LoggerUploadStatus_t Logger_GetUploadStatus(void) { return upload_status; }
uint32_t Logger_GetSavedCount(void) { return Storage_GetCount(); }
bool Logger_RetryUpload(void)
{
    if (logger_running || upload_status == LOGGER_UPLOAD_SENDING ||
        Storage_GetCount() == 0U) return false;
    upload_status = LOGGER_UPLOAD_WAITING_USB;
    upload_index = 0U;
    upload_header_sent = false;
    upload_line[0] = '\0';
    return true;
}
