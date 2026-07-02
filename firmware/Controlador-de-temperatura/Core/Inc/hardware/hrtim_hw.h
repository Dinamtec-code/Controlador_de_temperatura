#ifndef HRTIM_HW_H
#define HRTIM_HW_H

#include <stdint.h>
#include "main.h"

typedef enum
{
    HRTIM_OUTPUT_CH_TA1,
    HRTIM_OUTPUT_CH_TB1,
    HRTIM_OUTPUT_CH_TC1,
    HRTIM_OUTPUT_CH_TD1
} hrtim_output_t;

typedef struct PWM_CHANNEL
{
    hrtim_output_t output;
    uint16_t max_duty_cycle;
    uint16_t min_duty_cycle;
    uint16_t duty_cycle;
    uint16_t dead_time;
} pwm_channel_t;

typedef struct PWM_CONTROLLER
{
    pwm_channel_t channels[4];
} pwm_controller_t;

void hrtim_hw_init(void);
void hrtim_hw_set_duty(hrtim_output_t output, float duty);
void hrtim_hw_start(void);

#endif