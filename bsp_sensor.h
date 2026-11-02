#ifndef __BSP_SENSOR_H
#define __BSP_SENSOR_H

#include <stdint.h>

void BSP_Sensor_Init(void);
void BSP_Sensor_Read(uint16_t values[8]);
/* Returns 1 when the line is detected, otherwise 0. Position is -3500..3500. */
uint8_t BSP_Sensor_GetPosition(int32_t *position);

#endif
