#ifndef ADC_HW_H
#define ADC_HW_H

#include <stdint.h>

void adc_hw_init(void);
float adc_hw_read_temperature(void);

#endif