#include "hardware/adc_hw.h"
#include "stm32f3xx_hal.h"
#include "adc.h"

static float temperature_actual = 20.0f;
static int16_t adc_raw_temp = 0;

void adc_hw_init(void)
{
}

float adc_hw_read_temperature(void)
{
    static float bit2tempSlope = 330.0 / 4096.0;
    if (HAL_ADC_Start(&hadc2) == HAL_OK)
    {
        if (HAL_ADC_PollForConversion(&hadc2, 10) == HAL_OK)
        {
            adc_raw_temp = (int16_t)HAL_ADC_GetValue(&hadc2);
            temperature_actual = (adc_raw_temp * bit2tempSlope) * 0.1f + temperature_actual * 0.9f;
        }
    }
    return temperature_actual;
}