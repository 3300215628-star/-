/*
 * STM32F103C8T6 + SPL + 8-channel analog line sensor + TB6612FNG
 * Main control loop. Read README.md before wiring or flashing.
 */
#include "stm32f10x.h"
#include "main.h"
#include "bsp_motor.h"
#include "bsp_sensor.h"
#include "pid.h"

volatile uint32_t g_ms_ticks = 0;

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

void Delay_ms(uint32_t ms)
{
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {
        /* wait for SysTick */
    }
}

int main(void)
{
    int32_t position = 0;
    int16_t correction;
    int16_t left_speed;
    int16_t right_speed;
    uint8_t line_found;
    int8_t last_turn = 1;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);

    BSP_Motor_Init();
    BSP_Sensor_Init();
    PID_Reset();

    BSP_Motor_Stop();
    Delay_ms(300);

    while (1) {
        line_found = BSP_Sensor_GetPosition(&position);

        if (line_found) {
            if (position < 0) last_turn = -1;
            else if (position > 0) last_turn = 1;

            correction = PID_Calculate(position);
            left_speed = BASE_SPEED + correction;
            right_speed = BASE_SPEED - correction;

            if (left_speed > PWM_PERIOD) left_speed = PWM_PERIOD;
            if (left_speed < -PWM_PERIOD) left_speed = -PWM_PERIOD;
            if (right_speed > PWM_PERIOD) right_speed = PWM_PERIOD;
            if (right_speed < -PWM_PERIOD) right_speed = -PWM_PERIOD;

            BSP_Motor_Set(left_speed, right_speed);
        } else {
            /* Search slowly in the last known line direction. */
            PID_Reset();
            if (last_turn < 0) BSP_Motor_Set(-120, 120);
            else BSP_Motor_Set(120, -120);
        }

        Delay_ms(10);
    }
}
