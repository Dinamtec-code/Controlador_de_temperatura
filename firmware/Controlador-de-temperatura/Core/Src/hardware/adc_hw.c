#include "hardware/adc_hw.h"
#include "stm32f3xx_hal.h"
#include "main.h"
#include "adc.h"
#include <math.h>

float alpha = 0.01f;
float v2tem_coeff = 330.0f / 4096.0f;

static uint32_t adc_dma_buffer[ADC_HW_BUFFER_SIZE];

static volatile float adc_raw_ch1_filtered = 0.0f;
static volatile float adc_raw_ch2_filtered = 0.0f;

static void process_adc_samples(uint32_t start_idx, uint32_t count)
{
    // 'count' es la cantidad de datos de 32-bits en la mitad del buffer.
    // Avanzamos de a 2 para procesar solo el Rank 1 y saltear el Rank 2.
    for (uint32_t i = 0; i < count; i += 2)
    {
        uint32_t idx = start_idx + i;

        // Desempaquetado del Rank 1 (Temperatura)
        uint16_t val1 = (uint16_t)(adc_dma_buffer[idx] & 0xFFFF); // Master (ADC1)
        uint16_t val2 = (uint16_t)(adc_dma_buffer[idx] >> 16);    // Slave (ADC2)

        // Aplicamos el filtro EMA solo a los datos correctos
        adc_raw_ch1_filtered = (alpha * (float)val1) + ((1.0f - alpha) * adc_raw_ch1_filtered);
        adc_raw_ch2_filtered = (alpha * (float)val2) + ((1.0f - alpha) * adc_raw_ch2_filtered);

        /* * Si en el futuro se necesitás procesar el Rank 2, se hace sumando 1 al índice:
         * uint16_t val_aux1 = (uint16_t)(adc_dma_buffer[idx + 1] & 0xFFFF);
         */
    }
}

static void alpha_calculate(float fc_fs_ratio)
{
    /* Alpha calculation for a first-order low-pass filter:
     *  $\alpha = 1 - e^{-2\pi \frac{f_c}{F_s}} \approx 2\pi \frac{f_c}{F_s}$
     */
    alpha = 1 - expf(-2.0f * M_PI * fc_fs_ratio);
}

void adc_hw_init(void)
{
    adc_raw_ch1_filtered = 0.0f;
    adc_raw_ch2_filtered = 0.0f;
    alpha_calculate(50.0f / 72000.0f); // fc = 140Hz, fs = 72kHz
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, adc_dma_buffer, ADC_HW_BUFFER_SIZE);
}

float adc_hw_get_ch1(void)
{
    return adc_raw_ch1_filtered;
}

float adc_hw_get_ch2(void)
{
    return adc_raw_ch2_filtered;
}

float adc_hw_get_temp_ch1(void)
{
    return adc_raw_ch1_filtered * v2tem_coeff;
}

float adc_hw_get_temp_ch2(void)
{
    return adc_raw_ch2_filtered * v2tem_coeff;
}

float adc_hw_get_temp_diff(void)
{
    return v2tem_coeff * (adc_hw_get_ch1() - adc_hw_get_ch2());
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