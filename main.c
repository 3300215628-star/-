/*
 * STM32F103C8T6 + SPL + 8路模拟循迹 + TB6612FNG
 * 初始示例 main.c（用于建立工程框架，接线确认后再定稿）
 *
 * 默认引脚分配（可按实际接线修改）：
 *   ADC1：PA0~PA7 -> 8路模拟传感器 OUT1~OUT8
 *   TIM1_CH1：PA8 -> TB6612 PWMA（左电机）
 *   TIM1_CH2：PA9 -> TB6612 PWMB（右电机）
 *   PB12/PB13 -> AIN1/AIN2（左电机方向）
 *   PB14/PB15 -> BIN1/BIN2（右电机方向）
 *   PB0        -> STBY
 *
 * 注意：
 * 1. 本文件依赖 STM32 标准外设库（SPL）及正确配置的 72MHz 系统时钟。
 * 2. 黑线对应 ADC 高/低电平尚未确认，先通过 SENSOR_BLACK_HIGH 宏选择。
 * 3. 阈值、Kp、Kd、基础速度都需要结合实车调试。
 */

#include "stm32f10x.h"
#include <stdint.h>

#define PWM_PERIOD          999
#define PWM_MAX             PWM_PERIOD
#define BASE_SPEED          350
#define SENSOR_THRESHOLD    2000
#define SENSOR_BLACK_HIGH   1   /* 黑线数值高设为1；若黑线数值低，改为0 */

#define KP                  0.12f
#define KD                  0.45f

static volatile uint16_t g_adc_value[8];
static int32_t g_last_position = 0;
static uint32_t g_ms_ticks = 0;

static const int16_t g_sensor_weight[8] =
    {-3500, -2500, -1500, -500, 500, 1500, 2500, 3500};

void SysTick_Handler(void)
{
    g_ms_ticks++;
}

static void DelayMs(uint32_t ms)
{
    uint32_t start = g_ms_ticks;
    while ((g_ms_ticks - start) < ms) {
        /* wait */
    }
}

static void GPIO_Config(void)
{
    GPIO_InitTypeDef gpio;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |
                           RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_AFIO, ENABLE);

    /* PA0~PA7：模拟输入 */
    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
                    GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);

    /* PA8、PA9：TIM1 PWM 输出 */
    gpio.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9;
    gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    /* PB12~PB15：电机方向；PB0：STBY */
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_12 | GPIO_Pin_13 |
                    GPIO_Pin_14 | GPIO_Pin_15;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &gpio);

    GPIO_SetBits(GPIOB, GPIO_Pin_0); /* STBY=1，退出待机 */
}

static void ADC_Config(void)
{
    ADC_InitTypeDef adc;
    DMA_InitTypeDef dma;
    uint8_t rank;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); /* 72MHz / 6 = 12MHz */

    DMA_DeInit(DMA1_Channel1);
    DMA_StructInit(&dma);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)g_adc_value;
    dma.DMA_DIR = DMA_DIR_PeripheralSRC;
    dma.DMA_BufferSize = 8;
    dma.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    dma.DMA_MemoryInc = DMA_MemoryInc_Enable;
    dma.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    dma.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    dma.DMA_Mode = DMA_Mode_Circular;
    dma.DMA_Priority = DMA_Priority_High;
    dma.DMA_M2M = DMA_M2M_Disable;
    DMA_Init(DMA1_Channel1, &dma);
    DMA_Cmd(DMA1_Channel1, ENABLE);

    ADC_StructInit(&adc);
    adc.ADC_Mode = ADC_Mode_Independent;
    adc.ADC_ScanConvMode = ENABLE;
    adc.ADC_ContinuousConvMode = ENABLE;
    adc.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
    adc.ADC_DataAlign = ADC_DataAlign_Right;
    adc.ADC_NbrOfChannel = 8;
    ADC_Init(ADC1, &adc);

    for (rank = 0; rank < 8; rank++) {
        ADC_RegularChannelConfig(ADC1, (uint8_t)(ADC_Channel_0 + rank),
                                 (uint8_t)(rank + 1),
                                 ADC_SampleTime_55Cycles5);
    }

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) {
    }
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) {
    }

    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

static void TIM1_PWM_Config(void)
{
    TIM_TimeBaseInitTypeDef timebase;
    TIM_OCInitTypeDef oc;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);

    /* 72MHz / (71+1) / (999+1) = 1kHz */
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

    TIM_CtrlPWMOutputs(TIM1, ENABLE); /* 高级定时器必须开启主输出 */
    TIM_Cmd(TIM1, ENABLE);
}

static void Motor_SetOne(int16_t speed, uint16_t in1, uint16_t in2,
                         volatile uint32_t *ccr)
{
    uint16_t pwm;

    if (speed > PWM_MAX) speed = PWM_MAX;
    if (speed < -PWM_MAX) speed = -PWM_MAX;
    pwm = (uint16_t)((speed >= 0) ? speed : -speed);

    if (speed > 0) {
        GPIO_SetBits(GPIOB, in1);
        GPIO_ResetBits(GPIOB, in2);
    } else if (speed < 0) {
        GPIO_ResetBits(GPIOB, in1);
        GPIO_SetBits(GPIOB, in2);
    } else {
        GPIO_ResetBits(GPIOB, in1 | in2);
    }

    *ccr = pwm;
}

static void Motor_Set(int16_t left, int16_t right)
{
    Motor_SetOne(left, GPIO_Pin_12, GPIO_Pin_13, &TIM1->CCR1);
    Motor_SetOne(right, GPIO_Pin_14, GPIO_Pin_15, &TIM1->CCR2);
}

static uint16_t Sensor_GetStrength(uint8_t index)
{
    uint16_t value = g_adc_value[index];

#if SENSOR_BLACK_HIGH
    return (value > SENSOR_THRESHOLD) ? value : 0;
#else
    return (value < SENSOR_THRESHOLD) ? (uint16_t)(4095 - value) : 0;
#endif
}

/* 返回循迹位置：左负右正；没有检测到黑线时保持上一次位置 */
static int32_t Sensor_GetPosition(void)
{
    uint32_t sum = 0;
    int32_t weighted_sum = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        uint16_t strength = Sensor_GetStrength(i);
        sum += strength;
        weighted_sum += (int32_t)strength * g_sensor_weight[i];
    }

    if (sum == 0) {
        return g_last_position;
    }

    g_last_position = weighted_sum / (int32_t)sum;
    return g_last_position;
}

int main(void)
{
    int32_t position;
    int32_t error;
    int32_t last_error = 0;
    float correction;
    int16_t left_speed;
    int16_t right_speed;

    SystemInit();
    SystemCoreClockUpdate();
    SysTick_Config(SystemCoreClock / 1000);

    GPIO_Config();
    ADC_Config();
    TIM1_PWM_Config();

    Motor_Set(0, 0);
    DelayMs(200);

    while (1) {
        position = Sensor_GetPosition();
        error = position;

        correction = KP * (float)error +
                     KD * (float)(error - last_error);
        last_error = error;

        left_speed = (int16_t)(BASE_SPEED + correction);
        right_speed = (int16_t)(BASE_SPEED - correction);

        if (left_speed > PWM_MAX) left_speed = PWM_MAX;
        if (left_speed < -PWM_MAX) left_speed = -PWM_MAX;
        if (right_speed > PWM_MAX) right_speed = PWM_MAX;
        if (right_speed < -PWM_MAX) right_speed = -PWM_MAX;

        Motor_Set(left_speed, right_speed);
        DelayMs(10);
    }
}
