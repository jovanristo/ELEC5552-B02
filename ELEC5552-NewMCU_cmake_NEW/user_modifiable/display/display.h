#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_WIDTH   480U
#define DISPLAY_HEIGHT  320U

/* Requires lv_init() and lv_tick_set_cb() to be called first in main.c. */
bool Display_Init(void);

#endif /* DISPLAY_H */
