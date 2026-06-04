#include "tasks/task_control.h"
#include "control/pid_controller.h"
#include "hardware/adc_hw.h"
#include "hardware/hrtim_hw.h"

float def_temperatureSetpoint = 20.0f;
float def_pidKp = 0.0f;
float def_pidKi = 0.0f;
float def_pidKd = 0.0f;

void task_control(void)
{
    if (!(get_temp_pid_instance()->initialized))
    {
        pid_init(get_temp_pid_instance(), def_pidKp, def_pidKi, def_pidKd);
        pid_set_limits(get_temp_pid_instance(), 300, 65000.0f);
        pid_set_setpoint(get_temp_pid_instance(), def_temperatureSetpoint);
    }

    float temp = adc_hw_read_temperature();
    float output = pid_compute(get_temp_pid_instance(), temp);

    if (output >= 0)
    {
        hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, output);
        hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, 0.0f);
    }
    else
    {
        // hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, 0.0f);
        // hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, output);
    }
}
