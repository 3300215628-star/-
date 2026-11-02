#ifndef __BSP_MOTOR_H
#define __BSP_MOTOR_H

#include <stdint.h>

void BSP_Motor_Init(void);
void BSP_Motor_Set(int16_t left_speed, int16_t right_speed);
void BSP_Motor_Stop(void);

#endif
