#include "outputs.h"

#include "mx_dac1.h"

#include <math.h>
#include <stddef.h>


/* ---------------------------------------------------------
 * DAC handle
 * --------------------------------------------------------- */

static hal_dac_handle_t *dac1_handle = NULL;


/* ---------------------------------------------------------
 * Current commanded DAC state
 * --------------------------------------------------------- */

static uint16_t dac_raw = 0U;

static float dac_voltage_v = 0.0f;


/* ---------------------------------------------------------
 * Convert DAC voltage to raw code
 * --------------------------------------------------------- */

static uint16_t Outputs_VoltageToRaw(float voltage_v)
{
    float raw =
        (voltage_v / OUTPUT_DAC_REFERENCE_V)
        * (float)OUTPUT_DAC_LEVELS;


    if (raw <= 0.0f)
    {
        return 0U;
    }


    if (raw >= (float)OUTPUT_DAC_LEVELS)
    {
        return OUTPUT_DAC_MAX_RAW;
    }


    return (uint16_t)(raw + 0.5f);
}


/* ---------------------------------------------------------
 * Convert raw DAC code to voltage
 * --------------------------------------------------------- */

static float Outputs_RawToVoltage(uint16_t raw_value)
{
    return
        ((float)raw_value /
         (float)OUTPUT_DAC_LEVELS)
        * OUTPUT_DAC_REFERENCE_V;
}


/* ---------------------------------------------------------
 * Low-level hardware write
 * --------------------------------------------------------- */

static bool Outputs_WriteDACRaw(uint16_t raw_value)
{
    if (dac1_handle == NULL)
    {
        return false;
    }


    if (raw_value > OUTPUT_DAC_MAX_RAW)
    {
        return false;
    }


    if (HAL_DAC_SetChannelData(
            dac1_handle,
            HAL_DAC_CHANNEL_1,
            (uint32_t)raw_value) != HAL_OK)
    {
        return false;
    }


    return true;
}


/* ---------------------------------------------------------
 * Initialise outputs
 * --------------------------------------------------------- */

bool Outputs_Init(void)
{
    dac1_handle =
        mx_dac1_gethandle();


    if (dac1_handle == NULL)
    {
        return false;
    }


    /*
     * Load safe value before starting DAC.
     */
    if (HAL_DAC_SetChannelData(
            dac1_handle,
            HAL_DAC_CHANNEL_1,
            0U) != HAL_OK)
    {
        return false;
    }


    /*
     * Enable DAC channel.
     */
    if (HAL_DAC_StartChannel(
            dac1_handle,
            HAL_DAC_CHANNEL_1) != HAL_OK)
    {
        return false;
    }


    dac_raw = 0U;
    dac_voltage_v = 0.0f;


    return true;
}


/* ---------------------------------------------------------
 * CC output
 *
 * Relationship:
 *
 * 3.75 A -> 2.1875 V
 *
 * Therefore:
 *
 * DAC voltage =
 * current * (2.1875 / 3.75)
 * --------------------------------------------------------- */

bool Outputs_SetCurrent(float current_a)
{
    if (!isfinite(current_a))
    {
        return false;
    }


    if ((current_a < 0.0f) ||
        (current_a > OUTPUT_CC_MAX_CURRENT_A))
    {
        return false;
    }


    float dac_voltage =
        current_a *
        (OUTPUT_CC_MAX_DAC_V /
         OUTPUT_CC_MAX_CURRENT_A);


    uint16_t raw_value =
        Outputs_VoltageToRaw(dac_voltage);


    /*
     * Additional protection against exceeding
     * intended CC DAC operating range.
     */
    if (raw_value > OUTPUT_CC_MAX_RAW)
    {
        raw_value = OUTPUT_CC_MAX_RAW;
    }


    if (!Outputs_WriteDACRaw(raw_value))
    {
        return false;
    }


    dac_raw = raw_value;

    dac_voltage_v =
        Outputs_RawToVoltage(raw_value);


    return true;
}


/* ---------------------------------------------------------
 * CV output
 *
 * Relationship:
 *
 * DUT voltage / 60
 *
 * must equal
 *
 * DAC voltage * 6/25
 *
 * Therefore:
 *
 * DAC voltage =
 * DUT voltage * 25 / 360
 *
 * At 30 V:
 *
 * DAC = 2.08333 V
 * --------------------------------------------------------- */

bool Outputs_SetVoltage(float dut_voltage_v)
{
    if (!isfinite(dut_voltage_v))
    {
        return false;
    }


    if ((dut_voltage_v < 0.0f) ||
        (dut_voltage_v > OUTPUT_CV_MAX_VOLTAGE_V))
    {
        return false;
    }


    float dac_voltage =
        dut_voltage_v *
        (25.0f / 360.0f);


    uint16_t raw_value =
        Outputs_VoltageToRaw(dac_voltage);


    /*
     * Additional protection against exceeding
     * intended CV DAC operating range.
     */
    if (raw_value > OUTPUT_CV_MAX_RAW)
    {
        raw_value = OUTPUT_CV_MAX_RAW;
    }


    if (!Outputs_WriteDACRaw(raw_value))
    {
        return false;
    }


    dac_raw = raw_value;

    dac_voltage_v =
        Outputs_RawToVoltage(raw_value);


    return true;
}


/* ---------------------------------------------------------
 * Direct raw DAC command
 * --------------------------------------------------------- */

bool Outputs_SetDACRaw(uint16_t raw_value)
{
    if (raw_value > OUTPUT_DAC_MAX_RAW)
    {
        return false;
    }


    if (!Outputs_WriteDACRaw(raw_value))
    {
        return false;
    }


    dac_raw = raw_value;

    dac_voltage_v =
        Outputs_RawToVoltage(raw_value);


    return true;
}


/* ---------------------------------------------------------
 * Direct DAC voltage command
 * --------------------------------------------------------- */

bool Outputs_SetDACVoltage(float voltage_v)
{
    if (!isfinite(voltage_v))
    {
        return false;
    }


    if ((voltage_v < 0.0f) ||
        (voltage_v > OUTPUT_DAC_REFERENCE_V))
    {
        return false;
    }


    uint16_t raw_value =
        Outputs_VoltageToRaw(voltage_v);


    if (!Outputs_WriteDACRaw(raw_value))
    {
        return false;
    }


    dac_raw = raw_value;

    dac_voltage_v =
        Outputs_RawToVoltage(raw_value);


    return true;
}


/* ---------------------------------------------------------
 * Get current DAC code
 * --------------------------------------------------------- */

uint16_t Outputs_GetDACRaw(void)
{
    return dac_raw;
}


/* ---------------------------------------------------------
 * Get current DAC voltage
 * --------------------------------------------------------- */

float Outputs_GetDACVoltage(void)
{
    return dac_voltage_v;
}


/* ---------------------------------------------------------
 * Safe output state
 * --------------------------------------------------------- */

void Outputs_SetSafe(void)
{
    if (dac1_handle != NULL)
    {
        (void)HAL_DAC_SetChannelData(
            dac1_handle,
            HAL_DAC_CHANNEL_1,
            0U);
    }


    dac_raw = 0U;

    dac_voltage_v = 0.0f;
}