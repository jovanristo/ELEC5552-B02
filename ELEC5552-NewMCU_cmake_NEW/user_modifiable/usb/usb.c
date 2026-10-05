#include "usb.h"

#include <string.h>


/* USBX CDC ACM instance */
static UX_SLAVE_CLASS_CDC_ACM *cdc_acm = UX_NULL;


/* ---------------------------------------------------------
 * Called when PC connects and CDC becomes active
 * --------------------------------------------------------- */

void USB_CDC_Activate(void *cdc_instance)
{
    cdc_acm = (UX_SLAVE_CLASS_CDC_ACM *)cdc_instance;
}


/* ---------------------------------------------------------
 * Called when USB CDC disconnects
 * --------------------------------------------------------- */

void USB_CDC_Deactivate(void)
{
    cdc_acm = UX_NULL;
}


/* ---------------------------------------------------------
 * Check connection
 * --------------------------------------------------------- */

bool USB_IsConnected(void)
{
    return (cdc_acm != UX_NULL);
}


/* ---------------------------------------------------------
 * Send raw data to PC
 * --------------------------------------------------------- */

bool USB_Send(const uint8_t *data, uint32_t length)
{
    ULONG actual_length = 0U;

    if ((cdc_acm == UX_NULL) ||
        (data == NULL) ||
        (length == 0U))
    {
        return false;
    }

    UINT status =
        ux_device_class_cdc_acm_write_run(
            cdc_acm,
            (UCHAR *)data,
            (ULONG)length,
            &actual_length
        );

    return ((status == UX_STATE_NEXT) &&
            (actual_length == length));
}


/* ---------------------------------------------------------
 * Send text string to PC
 * --------------------------------------------------------- */

bool USB_SendString(const char *text)
{
    if (text == NULL)
    {
        return false;
    }

    return USB_Send(
        (const uint8_t *)text,
        (uint32_t)strlen(text)
    );
}