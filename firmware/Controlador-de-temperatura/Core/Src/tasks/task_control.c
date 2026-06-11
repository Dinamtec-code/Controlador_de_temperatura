#include "tasks/task_control.h"
#include "control/pid_controller.h"
#include "hardware/adc_hw.h"
#include "hardware/hrtim_hw.h"
#include "main.h"

float def_temperatureSetpoint = 25.0f;
float def_pidKp = 0.0f;
float def_pidKi = 0.0f;
float def_pidKd = 0.0f;

void task_control(void)
{
    pid_controller_t *pid_instance = get_temp_pid_instance();

    if (!(pid_instance->initialized))
    {
        pid_init(pid_instance, def_pidKp, def_pidKi, def_pidKd);
        pid_set_limits(pid_instance, -50.0f, 95.0f);
        pid_set_setpoint(pid_instance, def_temperatureSetpoint);

        hrtim_hw_init();
        adc_hw_init();
    }

    float temp = adc_hw_get_temp_diff();
    float output = pid_compute(pid_instance, temp);

    /* Update output state based on PID output */

    if (pid_controller_get_output(pid_instance, 0) == true)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    }

    if (pid_controller_get_output(pid_instance, 0) == true)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    }

    if (output < 0)
    {
        hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, 0);
        if (output >= pid_get_limit_min(pid_instance))
        {
            hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, output);
        }
        else
        {
            hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, pid_get_limit_min(pid_instance));
        }
    }
    else if (output >= 0)
    {
        hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, 0);
        if (output <= pid_get_limit_max(pid_instance))
        {
            hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, output);
        }
        else
        {
            hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, pid_get_limit_max(pid_instance));
        }
    }
}
