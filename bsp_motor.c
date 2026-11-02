#include "stm32f10x.h"
#include "main.h"
#include "bsp_motor.h"

static void Motor_SetOne(int16_t speed, uint16_t in1_pin, uint16_t in2_pin,
                         volatile uint32_t *compare)
{
    uint16_t duty;

    if (speed > PWM_PERIOD) speed = PWM_PERIOD;
    if (speed < -PWM_PERIOD) speed = -PWM_PERIOD;
    duty = (uint16_t)((speed >= 0) ? speed : -speed);

    if (speed > 0) {
        GPIO_SetBits(GPIOB, in1_pin);
        GPIO_ResetBits(GPIOB, in2_pin);
    } else if (speed < 0) {
        GPIO_ResetBits(GPIOB, in1_pin);
        GPIO_SetBits(GPIOB, in2_pin);
    } else {
        GPIO_ResetBits(GPIOB, in1_pin | in2_pin);
    }
    *compare = duty;
}

void BSP_Motor_Init(void)
{
    GPIO_InitTypeDef gpio;
    TIM_TimeBaseInitTypeDef timebase;
    TIM_OCInitTypeDef oc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO | RCC_APB2Periph_TIM1, ENABLE);

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = MOTOR_STBY_PIN | MOTOR_L_IN1_PIN | MOTOR_L_IN2_PIN |
                    MOTOR_R_IN1_PIN | MOTOR_R_IN2_PIN;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    GPIO_SetBits(MOTOR_STBY_GPIO, MOTOR_STBY_PIN);

    /* Assumes TIM1 input clock is 72 MHz: PWM frequency = 1 kHz. */
    TIM_TimeBaseStructInit(&timebase);
    timebase.TIM_Prescaler = 71;
    timebase.TIM_CounterMode = TIM_CounterMode_Up;
    timebase.TIM_Period = PWM_PERIOD;
    timebase.TIM_ClockDivision = TIM_CKD_DIV1;
    timebase.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &timebase);

    TIM_OCStructInit(&oc);
    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_OCPolarity = TIM_OCPolarity_High;
    oc.TIM_Pulse = 0;
    TIM_OC1Init(TIM1, &oc);
    TIM_OC2Init(TIM1, &oc);
    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
    BSP_Motor_Stop();
}

void BSP_Motor_Set(int16_t left_speed, int16_t right_speed)
{
    Motor_SetOne(left_speed, MOTOR_L_IN1_PIN, MOTOR_L_IN2_PIN, &TIM1->CCR1);
    Motor_SetOne(right_speed, MOTOR_R_IN1_PIN, MOTOR_R_IN2_PIN, &TIM1->CCR2);
}

void BSP_Motor_Stop(void)
{
    BSP_Motor_Set(0, 0);
}
