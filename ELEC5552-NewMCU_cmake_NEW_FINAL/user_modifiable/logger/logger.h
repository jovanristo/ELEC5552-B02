#ifndef LOGGER_H
#define LOGGER_H

#include <stdbool.h>
#include <stdint.h>


/*
 * Start logging.
 *
 * The run duration must already have been set using:
 *
 * Timers_RunTimerSet(hours, minutes, seconds);
 */
bool Logger_Start(uint32_t requested_sample_period_ms);


/* Called continuously from main loop */
void Logger_Update(void);


/* Stop logging and run timer */
void Logger_Stop(void);


/* Logger status */
bool Logger_IsRunning(void);


/* Requested period selected by user */
uint32_t Logger_GetRequestedSamplePeriod(void);


/* Actual period currently being used */
uint32_t Logger_GetSamplePeriod(void);


typedef enum
{
    LOGGER_UPLOAD_NONE = 0,
    LOGGER_UPLOAD_WAITING_USB,
    LOGGER_UPLOAD_SENDING,
    LOGGER_UPLOAD_COMPLETE
} LoggerUploadStatus_t;

LoggerUploadStatus_t Logger_GetUploadStatus(void);
uint32_t Logger_GetSavedCount(void);
/* Resend from the first line if the host was not capturing the transfer. */
bool Logger_RetryUpload(void);

#endif /* LOGGER_H */