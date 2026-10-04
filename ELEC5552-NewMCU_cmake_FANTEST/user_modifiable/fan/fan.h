#ifndef FAN_H
#define FAN_H

#include <stdint.h>
#include <stdbool.h>

bool Fan_Init(void);
void Fan_Update(void);

void Fan_SetDuty(float duty_percent);

float Fan_GetDuty(void);
uint32_t Fan_GetRPM(void);

#endif /* FAN_H */