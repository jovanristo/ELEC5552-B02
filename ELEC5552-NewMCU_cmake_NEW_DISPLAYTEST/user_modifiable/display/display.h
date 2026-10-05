#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdbool.h>
#include <stdint.h>

#define DISPLAY_WIDTH   480U
#define DISPLAY_HEIGHT  320U

bool Display_Init(void);

bool Display_FillScreen(uint8_t red,
                        uint8_t green,
                        uint8_t blue);

bool Display_DrawTest(void);

#endif /* DISPLAY_H */