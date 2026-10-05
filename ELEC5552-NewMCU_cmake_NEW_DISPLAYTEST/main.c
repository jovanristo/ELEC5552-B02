#include "main.h"
#include "display.h"

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

    if (!Display_DrawTest())
    {
        return -1;
    }

    while (1)
    {
    }
}