#include "hardware/adc_hw.h"
#include "stm32f3xx_hal.h"
#include "main.h"
#include "adc.h"
#include <math.h>

static const float alpha = 0.01f;

static uint32_t adc_dma_buffer[ADC_HW_BUFFER_SIZE];

static volatile float adc_raw_ch1_filtered = 0.0f;
static volatile float adc_raw_ch2_filtered = 0.0f;

static void process_adc_samples(uint32_t start_idx, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        uint32_t idx = start_idx + i;
        uint16_t val1 = (uint16_t)(adc_dma_buffer[idx] & 0xFFFF);
        uint16_t val2 = (uint16_t)(adc_dma_buffer[idx] >> 16);

        adc_raw_ch1_filtered = (alpha * (float)val1) + ((1.0f - alpha) * adc_raw_ch1_filtered);
        adc_raw_ch2_filtered = (alpha * (float)val2) + ((1.0f - alpha) * adc_raw_ch2_filtered);
    }
    //HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin); // Toggle LED for debugging
}

static inline void alpha_calculate(float fc_fs_ratio)
{
    /* Alpha calculation for a first-order low-pass filter:
     *  $\alpha = 1 - e^{-2\pi \frac{f_c}{F_s}} \approx 2\pi \frac{f_c}{F_s}$
     */
    alpha = 1 - expf(2.0f * M_PI * fc_fs_ratio);
}

void adc_hw_init(void)
{
    adc_raw_ch1_filtered = 0.0f;
    adc_raw_ch2_filtered = 0.0f;
    alpha_calculate(0.002f); // Example: fc = 140Hz, fs = 72kHz
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, adc_dma_buffer, ADC_HW_BUFFER_SIZE / 2);
}

float adc_hw_get_ch1(void)
{
    return adc_raw_ch1_filtered;
}

float adc_hw_get_ch2(void)
{
    return adc_raw_ch2_filtered;
}

float adc_hw_get_temp_diff(void)
{
    return 330.0f * (adc_hw_get_ch1() - adc_hw_get_ch2()) / 4095.0f;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        process_adc_samples(0, ADC_HW_BUFFER_SIZE / 2);
    }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        process_adc_samples(ADC_HW_BUFFER_SIZE / 2, ADC_HW_BUFFER_SIZE / 2);
    }
}