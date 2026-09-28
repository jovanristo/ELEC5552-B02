#include "timers.h"
#include "stm32_hal.h"


uint32_t Timers_GetMillis(void)
{
    return HAL_GetTick();
}


bool Timers_HasElapsed(uint32_t start_ms, uint32_t period_ms)
{
    return (uint32_t)(HAL_GetTick() - start_ms) >= period_ms;
}