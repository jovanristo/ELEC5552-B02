#ifndef CONTROL_H
#define CONTROL_H

#include <stdbool.h>


/* =========================================================
 * Electronic load states
 * ========================================================= */

typedef enum
{
    CONTROL_STATE_IDLE = 0,
    CONTROL_STATE_CC,
    CONTROL_STATE_CC_TO_CV,
    CONTROL_STATE_CV_STARTUP,
    CONTROL_STATE_CV

} ControlState_t;

bool Control_Start(void);

/* Initialise control state machine */
void Control_Init(void);


/* Run state machine */
void Control_Update(void);


/* Start a CC run */
bool Control_StartCC(void);


/* Start a CV run */
bool Control_StartCV(void);


/* Stop current run and return to IDLE */
void Control_Stop(void);


/* Get current state */
ControlState_t Control_GetState(void);


#endif /* CONTROL_H */