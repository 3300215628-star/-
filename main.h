#ifndef __MAIN_H
#define __MAIN_H

#include "stm32f10x.h"
#include <stdint.h>

/* ===== Hardware mapping: confirm against your actual wiring ===== */
#define MOTOR_STBY_GPIO       GPIOB
#define MOTOR_STBY_PIN        GPIO_Pin_0

#define MOTOR_L_IN1_GPIO      GPIOB
#define MOTOR_L_IN1_PIN       GPIO_Pin_12
#define MOTOR_L_IN2_GPIO      GPIOB
#define MOTOR_L_IN2_PIN       GPIO_Pin_13

#define MOTOR_R_IN1_GPIO      GPIOB
#define MOTOR_R_IN1_PIN       GPIO_Pin_14
#define MOTOR_R_IN2_GPIO      GPIOB
#define MOTOR_R_IN2_PIN       GPIO_Pin_15

#define MOTOR_L_PWM_CHANNEL   1u   /* TIM1_CH1 -> PA8 */
#define MOTOR_R_PWM_CHANNEL   2u   /* TIM1_CH2 -> PA9 */

#define PWM_PERIOD            999
#define BASE_SPEED            350
#define MAX_CORRECTION        300

/* Adjust after measuring the sensor outputs over white and black surfaces. */
#define SENSOR_THRESHOLD      2000
#define SENSOR_BLACK_HIGH     1   /* 1: black gives higher ADC; 0: black gives lower ADC */

#define PID_KP                0.12f
#define PID_KD                0.45f

void Delay_ms(uint32_t ms);

#endif
