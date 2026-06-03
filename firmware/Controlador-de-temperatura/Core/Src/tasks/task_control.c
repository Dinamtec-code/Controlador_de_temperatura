#include "tasks/task_control.h"
#include "control/pid_controller.h"
#include "hardware/adc_hw.h"
#include "hardware/hrtim_hw.h"

static pid_controller_t temp_pid;
static uint8_t pid_initialized = 0;

extern float temperatureSetpoint;
extern float pidKp, pidKi, pidKd;

void task_control(void)
{
    if (!pid_initialized) {
        pid_init(&temp_pid, pidKp, pidKi, pidKd);
        pid_set_limits(&temp_pid, -100.0f, 100.0f, -50.0f, 50.0f);
        pid_set_setpoint(&temp_pid, temperatureSetpoint);
        pid_initialized = 1;
    } else {
        pid_set_setpoint(&temp_pid, temperatureSetpoint);
        pid_set_parameters(&temp_pid, pidKp, pidKi, pidKd);
    }

    float temp = adc_hw_read_temperature();
    float output = pid_compute(&temp_pid, temp);

    if (output >= 0) {
        hrtim_hw_set_duty(HRTIM_OUTPUT_TA1, output);
        hrtim_hw_set_duty(HRTIM_OUTPUT_TB1, 0.0f);
    } else {
        hrtim_hw_set_duty(HRTIM_OUTPUT_TA1, 0.0f);
        hrtim_hw_set_duty(HRTIM_OUTPUT_TB1, -output);
    }
}