#include "hardware/adc_hw.h"
#include "stm32f3xx_hal.h"
#include "main.h"
#include "adc.h"
#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846f
#endif

#define MA_WINDOW_SIZE 20

typedef union
{
    uint32_t raw_32;
    struct
    {
        uint16_t ch1; // Master (ADC1) - Parte baja
        uint16_t ch2; // Slave (ADC2) - Parte alta
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

/* COEFICIENTE OPTIMIZADO:
   4096 (12-bits) * 4 (Sobremuestreo FIR) * 20 (Suma Móvil) = 327680.0f
*/
const float v2tem_coeff = 330.0f / 327680.0f;

// Estado del filtro EMA (Alta frecuencia)
static filter_state_t filter_nodes[ADC_FILTTER_ORDER];

// Estado del filtro de Media Móvil (Sincrónica - Filtro Notch 50Hz)
static float ma_buffer_ch1[MA_WINDOW_SIZE];
static float ma_buffer_ch2[MA_WINDOW_SIZE];
static uint8_t ma_idx = 0;
static float ma_sum_ch1 = 0.0f;
static float ma_sum_ch2 = 0.0f;

// Salidas finales del sistema (Suma pura de 20 muestras)
static volatile float adc_ma_final_ch1;
static volatile float adc_ma_final_ch2;

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

    adc_dual_t *adc_data = (adc_dual_t *)adc_dma_buffer;

    float loc_alpha = filt_coeff.alpha;
    float loc_beta = filt_coeff.beta;

    // Procesamiento del bloque de muestras del DMA
    for (uint32_t i = 0; i < count; i += 8)
    {
        uint32_t idx = start_idx + i;

        // Sobremuestreo x4 en enteros
        float v_ch1 = (float)(adc_data[idx].channels.ch1 + adc_data[idx + 2].channels.ch1 + adc_data[idx + 4].channels.ch1 + adc_data[idx + 6].channels.ch1);
        float v_ch2 = (float)(adc_data[idx].channels.ch2 + adc_data[idx + 2].channels.ch2 + adc_data[idx + 4].channels.ch2 + adc_data[idx + 6].channels.ch2);

        // Primer orden
        loc_ch1[0] = ema_filter(v_ch1, loc_ch1[0], loc_alpha, loc_beta);
        loc_ch2[0] = ema_filter(v_ch2, loc_ch2[0], loc_alpha, loc_beta);

        // Cascada del filtro
        for (int n = 1; n < ADC_FILTTER_ORDER; n++)
        {
            loc_ch1[n] = ema_filter(loc_ch1[n - 1], loc_ch1[n], loc_alpha, loc_beta);
            loc_ch2[n] = ema_filter(loc_ch2[n - 1], loc_ch2[n], loc_alpha, loc_beta);
        }
    }

    // 2. Guardamos el estado del EMA de vuelta en el array global
    for (int n = 0; n < ADC_FILTTER_ORDER; n++)
    {
        filter_nodes[n].ch1 = loc_ch1[n];
        filter_nodes[n].ch2 = loc_ch2[n];
    }

    // 3. PROCESAMIENTO SINCRÓNICO: Filtro Notch de 50Hz (Ejecutado cada 1ms con el DMA)
    // Extraemos la última salida de la cascada del EMA
    float last_ema_ch1 = loc_ch1[ADC_FILTTER_ORDER - 1];
    float last_ema_ch2 = loc_ch2[ADC_FILTTER_ORDER - 1];

    // Restamos el valor antiguo del buffer circular
    ma_sum_ch1 -= ma_buffer_ch1[ma_idx];
    ma_sum_ch2 -= ma_buffer_ch2[ma_idx];

    // Insertamos el nuevo dato del EMA
    ma_buffer_ch1[ma_idx] = last_ema_ch1;
    ma_buffer_ch2[ma_idx] = last_ema_ch2;

    // Sumamos el nuevo impacto
    ma_sum_ch1 += last_ema_ch1;
    ma_sum_ch2 += last_ema_ch2;

    // Avanzamos el índice circular
    ma_idx++;
    if (ma_idx >= MA_WINDOW_SIZE)
    {
        ma_idx = 0;
    }

    // 4. Publicamos las SUMAS PURAS directamente a las variables globales volatile
    adc_ma_final_ch1 = ma_sum_ch1;
    adc_ma_final_ch2 = ma_sum_ch2;
}

static void alpha_calculate(float fc_fs_ratio, int orden)
{
    filt_coeff.alpha = 1.0f - expf(-2.0f * M_PI * fc_fs_ratio / sqrtf(powf(2.0f, 1.0f / (float)orden) - 1.0f));
    filt_coeff.beta = 1.0f - filt_coeff.alpha;
}

void adc_hw_init(void)
{
    memset((void *)filter_nodes, 0, ADC_FILTTER_ORDER * sizeof(filter_state_t));

    // Limpieza de las estructuras de la media móvil sincrónica
    memset((void *)ma_buffer_ch1, 0, sizeof(ma_buffer_ch1));
    memset((void *)ma_buffer_ch2, 0, sizeof(ma_buffer_ch2));
    ma_idx = 0;
    ma_sum_ch1 = 0.0f;
    ma_sum_ch2 = 0.0f;
    adc_ma_final_ch1 = 0.0f;
    adc_ma_final_ch2 = 0.0f;

    // Si el buffer total tarda 2ms en llenarse (1ms por mitad), la frecuencia
    // de cortes/muestras dentro de process_adc_samples depende de cuántos sub-bloques procesas.
    // Asumiendo que la tasa efectiva de actualización interna del EMA es de 18kHz:
    alpha_calculate(12.5f / 18000.0f, ADC_FILTTER_ORDER);

    HAL_ADCEx_MultiModeStart_DMA(&hadc1, adc_dma_buffer, ADC_HW_BUFFER_SIZE);
}

// Funciones Getter apuntando a las sumas acumuladas estables
float adc_hw_get_ch1(void) { return adc_ma_final_ch1; }
float adc_hw_get_ch2(void) { return adc_ma_final_ch2; }

float adc_hw_get_temp_ch1(void) { return adc_ma_final_ch1 * v2tem_coeff; }
float adc_hw_get_temp_ch2(void) { return adc_ma_final_ch2 * v2tem_coeff; }
float adc_hw_get_temp_diff(void) { return v2tem_coeff * (adc_ma_final_ch1 - adc_ma_final_ch2); }

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
