#include "main.h"
#include "display.h"

#include "lvgl.h"

int main(void)
{
    if (mx_system_init() != SYSTEM_OK)
    {
        return -1;
    }


    if (!Display_Init())
    {
        return -1;
    }


    while (1)
    {
        lv_timer_handler();

        HAL_Delay(5U);
    }
}