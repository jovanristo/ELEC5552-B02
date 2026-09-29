#include "fan.h"

#include "inputs.h"
#include "timers.h"
#include "mx_tim1.h"
#include "mx_tim8.h"


/* Internal/heatsink thermistors:
 * Thermistors 1, 2, 3, 8, 9, 10, 11, 12
 *
 * Thermistors 4, 5, 6, 7 are external/DUT thermistors
 * and are not used for fan control.
 */
static const uint8_t internal_thermistor_indices[] =
{
    0U,  /* Thermistor 1  */
    1U,  /* Thermistor 2  */
    2U,  /* Thermistor 3  */
    7U,  /* Thermistor 8  */
    8U,  /* Thermistor 9  */
    9U,  /* Thermistor 10 */
    10U, /* Thermistor 11 */
    11U  /* Thermistor 12 */
};

#define NUM_INTERNAL_THERMISTORS \
    (sizeof(internal_thermistor_indices) / sizeof(internal_thermistor_indices[0]))


/* Fan temperature range */
#define FAN_TEMP_LOW_C               30.0f
#define FAN_TEMP_HIGH_C              70.0f

/* Minimum fan speed */
#define FAN_MIN_DUTY                 30.0f

#define FAN_MIN_RPM                  600U
#define FAN_MAX_RPM                  3000U

/* Tachometer */
#define FAN_TACH_PERIOD_MS           1000U
#define FAN_PULSES_PER_REV           2U


static float fan_duty = 0.0f;
static uint32_t fan_rpm = 0U;
static uint32_t last_tach_time = 0U;


/* ---------------------------------------------------------
 * Initialise fan PWM and tach counter
 * --------------------------------------------------------- */

bool Fan_Init(void)
{
    hal_tim_handle_t *tim1 = mx_tim1_gethandle();
    hal_tim_handle_t *tim8 = mx_tim8_gethandle();

    if ((tim1 == NULL) || (tim8 == NULL))
    {
        return false;
    }

    /* Start tach pulse counter */
    HAL_TIM_SetCounter(tim1, 0U);

    if (HAL_TIM_Start(tim1) != HAL_OK)
    {
        return false;
    }

    /* Start PWM timer */
    if (HAL_TIM_Start(tim8) != HAL_OK)
    {
        return false;
    }

    if (HAL_TIM_OC_StartChannel(tim8, HAL_TIM_CHANNEL_1) != HAL_OK)
    {
        return false;
    }

    /* Safe startup speed */
    Fan_SetDuty(100.0f);

    last_tach_time = Timers_GetMillis();

    return true;
}


/* ---------------------------------------------------------
 * Main fan update
 * --------------------------------------------------------- */

void Fan_Update(void)
{
    const Inputs_t *inputs = Inputs_Get();

    float hottest_temp = 0.0f;
    bool valid_temp = false;


    /* Find hottest internal/heatsink thermistor */
    for (uint32_t i = 0U; i < NUM_INTERNAL_THERMISTORS; i++)
    {
        uint8_t index = internal_thermistor_indices[i];

        if (inputs->thermistor_valid[index])
        {
            if ((!valid_temp) ||
                (inputs->thermistor_temp_c[index] > hottest_temp))
            {
                hottest_temp = inputs->thermistor_temp_c[index];
            }

            valid_temp = true;
        }
    }


    /* -----------------------------------------------------
     * Fan temperature control
     * ----------------------------------------------------- */

    if (!valid_temp)
    {
        /* Sensor problem -> run fan at full speed */
        Fan_SetDuty(100.0f);
    }
    else if (hottest_temp <= FAN_TEMP_LOW_C)
    {
        Fan_SetDuty(FAN_MIN_DUTY);
    }
    else if (hottest_temp >= FAN_TEMP_HIGH_C)
    {
        Fan_SetDuty(100.0f);
    }
    else
    {
        /*
         * Linear fan curve:
         *
         * 30 C -> 30%
         * 70 C -> 100%
         */

        float fraction =
            (hottest_temp - FAN_TEMP_LOW_C)
            /
            (FAN_TEMP_HIGH_C - FAN_TEMP_LOW_C);

        float duty =
            FAN_MIN_DUTY
            +
            fraction * (100.0f - FAN_MIN_DUTY);

        Fan_SetDuty(duty);
    }


    /* -----------------------------------------------------
     * Tachometer measurement
     * ----------------------------------------------------- */

    if (Timers_HasElapsed(last_tach_time,
                          FAN_TACH_PERIOD_MS))
    {
        hal_tim_handle_t *tim1 = mx_tim1_gethandle();

        uint32_t now = Timers_GetMillis();
        uint32_t elapsed_ms = now - last_tach_time;

        uint32_t pulses = HAL_TIM_GetCounter(tim1);

        HAL_TIM_SetCounter(tim1, 0U);


        if (elapsed_ms > 0U)
        {
            fan_rpm =
                (uint32_t)(
                    ((uint64_t)pulses * 60000ULL)
                    /
                    ((uint64_t)FAN_PULSES_PER_REV *
                     elapsed_ms)
                );
        }

        last_tach_time = now;
    }
}


/* ---------------------------------------------------------
 * Set PWM duty cycle
 * --------------------------------------------------------- */

void Fan_SetDuty(float duty_percent)
{
    hal_tim_handle_t *tim8 = mx_tim8_gethandle();

    /* Limit to 0-100% */
    if (duty_percent < 0.0f)
    {
        duty_percent = 0.0f;
    }

    if (duty_percent > 100.0f)
    {
        duty_percent = 100.0f;
    }


    uint32_t period = HAL_TIM_GetPeriod(tim8);

    uint32_t pulse =
        (uint32_t)(
            ((period + 1U) * duty_percent)
            / 100.0f
        );


    HAL_TIM_OC_SetCompareUnitPulse(
        tim8,
        HAL_TIM_OC_COMPARE_UNIT_1,
        pulse
    );


    fan_duty = duty_percent;
}


/* ---------------------------------------------------------
 * Getters
 * --------------------------------------------------------- */

float Fan_GetDuty(void)
{
    return fan_duty;
}


uint32_t Fan_GetRPM(void)
{
    return fan_rpm;
}