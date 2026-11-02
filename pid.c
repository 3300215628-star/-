#include "main.h"
#include "pid.h"

static int32_t s_last_error = 0;

void PID_Reset(void)
{
    s_last_error = 0;
}

int16_t PID_Calculate(int32_t error)
{
    float output = PID_KP * (float)error +
                   PID_KD * (float)(error - s_last_error);
    s_last_error = error;

    if (output > MAX_CORRECTION) output = MAX_CORRECTION;
    if (output < -MAX_CORRECTION) output = -MAX_CORRECTION;
    return (int16_t)output;
}
