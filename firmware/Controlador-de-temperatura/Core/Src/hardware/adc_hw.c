#include "hardware/adc_hw.h"
#include "stm32f3xx_hal.h"
#include "main.h"
#include "adc.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

typedef union
{
    uint32_t raw_32;
    struct
    {
        uint16_t val1; // Master (ADC1) - Parte baja
        uint16_t val2; // Slave (ADC2) - Parte alta
    } channels;
} adc_dual_t;

typedef struct
{
    float ch1;
    float ch2;
} filter_state_t;

typedef struct
{
    float alpha;
    float beta;
} filter_coeff_t;

static uint32_t adc_dma_buffer[ADC_HW_BUFFER_SIZE];

static filter_coeff_t filt_coeff;

const float v2tem_coeff = 330.0f / 4096.0f;

static filter_state_t filter_nodes[ADC_FILTTER_ORDER];

static volatile float adc_final_ch1;
static volatile float adc_final_ch2;

static inline float ema_filter(float x, float y, float a, float b)
{
    return (x * a) + (y * b);
}

static void process_adc_samples(uint32_t start_idx, uint32_t count)
{
    // 1. Cargamos el estado global a registros locales (rápido)
    float loc_ch1[ADC_FILTTER_ORDER];
    float loc_ch2[ADC_FILTTER_ORDER];

    for (int n = 0; n < ADC_FILTTER_ORDER; n++)
    {
        loc_ch1[n] = filter_nodes[n].ch1;
        loc_ch2[n] = filter_nodes[n].ch2;
    }

    // Cache local de las constantes para asegurar que se queden en los registros de la FPU
    float loc_alpha = filt_coeff.alpha;
    float loc_beta = filt_coeff.beta;

    // 'count' es la cantidad de datos de 32-bits en la mitad del buffer.
    for (uint32_t i = 0; i < count; i += 2)
    {
        uint32_t idx = start_idx + i;

        // Desempaquetado del Rank 1 (Más eficiente leyendo la variable local 'raw_data')
        adc_dual_t adc_data = {.raw_32 = adc_dma_buffer[idx]};

        float val1 = (float)adc_data.channels.val1;
        float val2 = (float)adc_data.channels.val2;

        // Primer orden
        loc_ch1[0] = ema_filter(val1, loc_ch1[0], loc_alpha, loc_beta);
        loc_ch2[0] = ema_filter(val2, loc_ch2[0], loc_alpha, loc_beta);

        // Cascada del filtro
        // Variables locales y un tamaño estático, el compilador (con -O2/-O3)
        // "desenrollar" (unroll) este bucle evitando saltos condicionales.
        for (int n = 1; n < ADC_FILTTER_ORDER; n++)
        {
            loc_ch1[n] = ema_filter(loc_ch1[n - 1], loc_ch1[n], loc_alpha, loc_beta);
            loc_ch2[n] = ema_filter(loc_ch2[n - 1], loc_ch2[n], loc_alpha, loc_beta);
        }
    }

    // 2. Guardamos el estado local de vuelta en el array global
    for (int n = 0; n < ADC_FILTTER_ORDER; n++)
    {
        filter_nodes[n].ch1 = loc_ch1[n];
        filter_nodes[n].ch2 = loc_ch2[n];
    }

    // 3. Actualizamos las salidas 'volatile' para el resto del sistema
    adc_final_ch1 = loc_ch1[ADC_FILTTER_ORDER - 1];
    adc_final_ch2 = loc_ch2[ADC_FILTTER_ORDER - 1];
}

static void alpha_calculate(float fc_fs_ratio, int orden)
{
    // Usamos las funciones "f" para forzar cálculos en Single Precision
    filt_coeff.alpha = 1.0f - expf(-2.0f * M_PI * fc_fs_ratio / sqrtf(powf(2.0f, 1.0f / (float)orden) - 1.0f));
    filt_coeff.beta = 1.0f - filt_coeff.alpha; // Precalculamos el complemento una única vez
}

void adc_hw_init(void)
{
    memset((void *)filter_nodes, 0, ADC_FILTTER_ORDER * sizeof(filter_state_t));

    alpha_calculate(10.0f / 72000.0f, ADC_FILTTER_ORDER);
    HAL_ADCEx_MultiModeStart_DMA(&hadc1, adc_dma_buffer, ADC_HW_BUFFER_SIZE);
}

// Funciones Getter apuntando a las variables volatile
float adc_hw_get_ch1(void) { return adc_final_ch1; }
float adc_hw_get_ch2(void) { return adc_final_ch2; }
float adc_hw_get_temp_ch1(void) { return adc_final_ch1 * v2tem_coeff; }
float adc_hw_get_temp_ch2(void) { return adc_final_ch2 * v2tem_coeff; }
float adc_hw_get_temp_diff(void) { return v2tem_coeff * (adc_final_ch1 - adc_final_ch2); }

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