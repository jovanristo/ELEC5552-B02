#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdbool.h>
#include <stdint.h>


/* ---------------------------------------------------------
 * DAC hardware
 * --------------------------------------------------------- */

#define OUTPUT_DAC_LEVELS        4096U
#define OUTPUT_DAC_MAX_RAW       4095U
#define OUTPUT_DAC_REFERENCE_V   2.5f


/* ---------------------------------------------------------
 * CC limits
 *
 * 3.75 A -> 2.1875 V DAC -> raw 3584
 * --------------------------------------------------------- */

#define OUTPUT_CC_MAX_CURRENT_A  3.75f
#define OUTPUT_CC_MAX_DAC_V      2.1875f
#define OUTPUT_CC_MAX_RAW        3584U


/* ---------------------------------------------------------
 * CV limits
 *
 * 30 V DUT -> 2.08333 V DAC -> raw ~3413
 * --------------------------------------------------------- */

#define OUTPUT_CV_MAX_VOLTAGE_V  30.0f
#define OUTPUT_CV_MAX_DAC_V      2.083333f
#define OUTPUT_CV_MAX_RAW        3413U


/* ---------------------------------------------------------
 * Initialise DAC
 * --------------------------------------------------------- */

bool Outputs_Init(void);


/* ---------------------------------------------------------
 * CC command
 *
 * Input:
 * desired stage current in amps
 *
 * Valid range:
 * 0 -> 3.75 A
 * --------------------------------------------------------- */

bool Outputs_SetCurrent(float current_a);


/* ---------------------------------------------------------
 * CV command
 *
 * Input:
 * desired DUT voltage in volts
 *
 * Valid range:
 * 0 -> 30 V
 * --------------------------------------------------------- */

bool Outputs_SetVoltage(float dut_voltage_v);


/* ---------------------------------------------------------
 * Direct DAC access
 *
 * Mainly useful for testing/debugging.
 * --------------------------------------------------------- */

bool Outputs_SetDACRaw(uint16_t raw_value);

bool Outputs_SetDACVoltage(float voltage_v);


/* ---------------------------------------------------------
 * Read current output command
 * --------------------------------------------------------- */

uint16_t Outputs_GetDACRaw(void);

float Outputs_GetDACVoltage(void);


/* ---------------------------------------------------------
 * Safe state
 * --------------------------------------------------------- */

void Outputs_SetSafe(void);

/* ---------------------------------------------------------
 * Set clamp
 * --------------------------------------------------------- */
void Outputs_ClampEnable(void);
void Outputs_ClampDisable(void);

/* ---------------------------------------------------------
 * Select mode
 * --------------------------------------------------------- */
void Outputs_SelectCC(void);
void Outputs_SelectCV(void);


#endif /* OUTPUTS_H */