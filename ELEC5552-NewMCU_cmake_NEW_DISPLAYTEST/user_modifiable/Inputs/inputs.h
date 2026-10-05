#ifndef INPUTS_H
#define INPUTS_H

#include <stdbool.h>
#include <stdint.h>


/* Number of thermistor channels connected to ADC2 */
#define NUM_THERMISTORS 12U


typedef struct
{
    /* =====================================================
     * Buttons
     * ===================================================== */

    bool button_0;
    bool button_1;
    bool button_2;
    bool button_3;
    bool button_4;
    bool button_5;
    bool button_6;
    bool button_7;
    bool button_8;
    bool button_9;

    bool button_up;
    bool button_down;
    bool button_left;
    bool button_right;
    bool button_ok;    

    /* =====================================================
     * USB
     * ===================================================== */

    bool usb_vbus_present;


    /* =====================================================
     * ADC1 raw electrical measurements
     * ===================================================== */

    uint16_t i_adc_side12_raw;   /* PA3 / ADC1_IN3 */
    uint16_t i_adc_side21_raw;   /* PA6 / ADC1_IN6 */
    uint16_t i_adc_side11_raw;   /* PA7 / ADC1_IN7 */
    uint16_t i_adc_side22_raw;   /* PC0 / ADC1_IN8 */
    uint16_t dut_adc_raw;        /* PA5 / ADC1_IN5 */



    /* =====================================================
     * Converted electrical measurements
     * ===================================================== */

    float current_side12_a;
    float current_side21_a;
    float current_side11_a;
    float current_side22_a;
    float total_current_a;
    float dut_voltage_v;
    float power_w;


    /* =====================================================
     * ADC2 temperature measurements
     * ===================================================== */

    uint16_t thermistor_raw[NUM_THERMISTORS];
    float thermistor_temp_c[NUM_THERMISTORS];
    bool thermistor_valid[NUM_THERMISTORS];

} Inputs_t;


/* Initialise the input-processing system */
void Inputs_Init(void);


/* Read and update all inputs */
void Inputs_Update(void);


/* Access the latest input values */
const Inputs_t *Inputs_Get(void);


#endif /* INPUTS_H */