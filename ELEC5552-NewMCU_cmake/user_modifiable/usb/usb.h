#ifndef USB_H
#define USB_H

#include <stdbool.h>
#include <stdint.h>

#include "ux_api.h"
#include "ux_device_class_cdc_acm.h"

/* Called by generated USBX callbacks */
void USB_CDC_Activate(void *cdc_instance);
void USB_CDC_Deactivate(void);

/* Application USB functions */
bool USB_IsConnected(void);

bool USB_Send(const uint8_t *data, uint32_t length);
bool USB_SendString(const char *text);

#endif /* USB_H */

// TEST : if (USB_IsConnected())
//  {
//    USB_SendString("Electronic load connected\r\n");
//  }