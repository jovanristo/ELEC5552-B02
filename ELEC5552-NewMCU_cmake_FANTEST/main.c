#include "main.h"

#include "fan.h"

int main(void)
{
    /*
     * Initialise MCU and all CubeMX2 peripherals.
     */
    if (mx_system_init() != SYSTEM_OK)
    {
        return (-1);
    }
    else
    {
        /*
         * Initialise fan PWM.
         */
        if (!Fan_Init())
        {
            return (-1);
        }

        /*
         * Start fan at full speed.
         */
        Fan_SetDuty(100.0f);
        HAL_Delay(1000);

        /*
         * Test different duty cycles.
         */
        Fan_SetDuty(30.0f);
        HAL_Delay(3000);

        Fan_SetDuty(50.0f);
        HAL_Delay(3000);

        Fan_SetDuty(75.0f);
        HAL_Delay(3000);

        Fan_SetDuty(100.0f);
        HAL_Delay(3000);

        /*
         * Stop fan.
         */
        Fan_SetDuty(0.0f);

        /*
         * Remain stopped.
         */
        while (1)
        {
        }
    }
}