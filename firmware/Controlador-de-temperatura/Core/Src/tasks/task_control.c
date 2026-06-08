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
    if (!(get_temp_pid_instance()->initialized))
    {
        pid_init(get_temp_pid_instance(), def_pidKp, def_pidKi, def_pidKd);
        pid_set_limits(get_temp_pid_instance(), 0.5f, 49.5f);
        pid_set_setpoint(get_temp_pid_instance(), def_temperatureSetpoint);

        hrtim_hw_start();
    }

    if (get_temp_pid_instance()->output_state[0] == true)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);
    }

    if (get_temp_pid_instance()->output_state[1] == true)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_RESET);
    }
    else
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, GPIO_PIN_SET);
    }

    float temp = adc_hw_get_temp_ch1();
    float output = pid_compute(get_temp_pid_instance(), temp);

    if (output >= 0)
    {
        hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, output);
        hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, output);
    }
    else
    {
        // hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TA1, 0.0f);
        // hrtim_hw_set_duty(HRTIM_OUTPUT_CH_TB1, output);
    }
}
