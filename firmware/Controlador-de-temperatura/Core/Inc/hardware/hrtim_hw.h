#ifndef HRTIM_HW_H
#define HRTIM_HW_H

#include <stdint.h>

typedef enum {
    HRTIM_OUTPUT_CH_TA1,
    HRTIM_OUTPUT_CH_TB1
} hrtim_output_t;

void hrtim_hw_init(void);
void hrtim_hw_set_duty(hrtim_output_t output, float duty);
void hrtim_hw_start(void);

#endif