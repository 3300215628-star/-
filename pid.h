#ifndef __PID_H
#define __PID_H

#include <stdint.h>

void PID_Reset(void);
int16_t PID_Calculate(int32_t error);

#endif
