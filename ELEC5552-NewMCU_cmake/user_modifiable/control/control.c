#include "control.h"


/* =========================================================
 * Current state
 * ========================================================= */

static ControlState_t control_state;


/* =========================================================
 * Initialise state machine
 * ========================================================= */

void Control_Init(void)
{
    control_state = CONTROL_STATE_IDLE;
}


/* =========================================================
 * Main state machine
 * ========================================================= */

void Control_Update(void)
{
    switch (control_state)
    {
        /* -------------------------------------------------
         * IDLE
         * ------------------------------------------------- */
        case CONTROL_STATE_IDLE:
        {
            /*
             * Electronic load is inactive.
             *
             * Safe-disable logic and DAC = 0
             * will be added next.
             */

            break;
        }


        /* -------------------------------------------------
         * CONSTANT CURRENT
         * ------------------------------------------------- */
        case CONTROL_STATE_CC:
        {
            /*
             * CC operation will be added later.
             */

            break;
        }


        /* -------------------------------------------------
         * CONSTANT VOLTAGE
         * ------------------------------------------------- */
        case CONTROL_STATE_CV:
        {
            /*
             * CV operation will be added later.
             */

            break;
        }


        /* -------------------------------------------------
         * Invalid state
         * ------------------------------------------------- */
        default:
        {
            control_state = CONTROL_STATE_IDLE;

            break;
        }
    }
}


/* =========================================================
 * Start CC
 * ========================================================= */

bool Control_StartCC(void)
{
    /*
     * Only allow a new run from IDLE.
     */
    if (control_state != CONTROL_STATE_IDLE)
    {
        return false;
    }


    control_state = CONTROL_STATE_CC;

    return true;
}


/* =========================================================
 * Start CV
 * ========================================================= */

bool Control_StartCV(void)
{
    /*
     * Only allow a new run from IDLE.
     */
    if (control_state != CONTROL_STATE_IDLE)
    {
        return false;
    }


    control_state = CONTROL_STATE_CV;

    return true;
}


/* =========================================================
 * Stop run
 * ========================================================= */

void Control_Stop(void)
{
    control_state = CONTROL_STATE_IDLE;
}


/* =========================================================
 * Get current state
 * ========================================================= */

ControlState_t Control_GetState(void)
{
    return control_state;
}