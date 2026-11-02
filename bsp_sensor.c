#include "stm32f10x.h"
#include "main.h"
#include "bsp_sensor.h"

static volatile uint16_t s_adc_values[8];
static const int16_t s_weights[8] = {
    -3500, -2500, -1500, -500, 500, 1500, 2500, 3500
};
static int32_t s_last_position = 0;

void BSP_Sensor_Init(void)
{
    GPIO_InitTypeDef gpio;
    ADC_InitTypeDef adc;
    DMA_InitTypeDef dma;
    uint8_t i;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    RCC_ADCCLKConfig(RCC_PCLK2_Div6); /* 72 MHz / 6 = 12 MHz */

    GPIO_StructInit(&gpio);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3 |
                    GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_6 | GPIO_Pin_7;
    gpio.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_Init(GPIOA, &gpio);

    DMA_DeInit(DMA1_Channel1);
    DMA_StructInit(&dma);
    dma.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    dma.DMA_MemoryBaseAddr = (uint32_t)s_adc_values;
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

    for (i = 0; i < 8; i++) {
        ADC_RegularChannelConfig(ADC1, (uint8_t)(ADC_Channel_0 + i),
                                 (uint8_t)(i + 1), ADC_SampleTime_55Cycles5);
    }

    ADC_DMACmd(ADC1, ENABLE);
    ADC_Cmd(ADC1, ENABLE);
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1)) {}
    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1)) {}
    ADC_SoftwareStartConvCmd(ADC1, ENABLE);
}

void BSP_Sensor_Read(uint16_t values[8])
{
    uint8_t i;
    for (i = 0; i < 8; i++) values[i] = s_adc_values[i];
}

static uint16_t Sensor_GetBlackStrength(uint16_t raw)
{
#if SENSOR_BLACK_HIGH
    return (raw > SENSOR_THRESHOLD) ? (uint16_t)(raw - SENSOR_THRESHOLD) : 0;
#else
    return (raw < SENSOR_THRESHOLD) ? (uint16_t)(SENSOR_THRESHOLD - raw) : 0;
#endif
}

uint8_t BSP_Sensor_GetPosition(int32_t *position)
{
    uint32_t strength_sum = 0;
    int32_t weighted_sum = 0;
    uint8_t i;

    for (i = 0; i < 8; i++) {
        uint16_t strength = Sensor_GetBlackStrength(s_adc_values[i]);
        strength_sum += strength;
        weighted_sum += (int32_t)strength * s_weights[i];
    }

    if (strength_sum == 0) {
        *position = s_last_position;
        return 0;
    }

    s_last_position = weighted_sum / (int32_t)strength_sum;
    *position = s_last_position;
    return 1;
}
