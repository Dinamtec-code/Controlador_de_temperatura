#ifndef ADC_HW_H
#define ADC_HW_H

#include <stdint.h>

#define ADC_HW_BUFFER_SIZE 64

void adc_hw_init(void);

float adc_hw_get_ch1(void);
float adc_hw_get_ch2(void);
float adc_hw_get_temp_diff(void);

#endif