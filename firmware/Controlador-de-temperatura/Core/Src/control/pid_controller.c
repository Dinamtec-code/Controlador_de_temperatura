#include "control/pid_controller.h"

void pid_init(pid_controller_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
    pid->setpoint = 0.0f;
    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
    pid->output_limit_min = -100.0f;
    pid->output_limit_max = 100.0f;
    pid->integral_limit_min = -50.0f;
    pid->integral_limit_max = 50.0f;
    pid->initialized = 1;
}

void pid_set_parameters(pid_controller_t *pid, float kp, float ki, float kd)
{
    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;
}

void pid_set_limits(pid_controller_t *pid, float out_min, float out_max, float int_min, float int_max)
{
    pid->output_limit_min = out_min;
    pid->output_limit_max = out_max;
    pid->integral_limit_min = int_min;
    pid->integral_limit_max = int_max;
}

void pid_set_setpoint(pid_controller_t *pid, float setpoint)
{
    pid->setpoint = setpoint;
}

float pid_compute(pid_controller_t *pid, float input)
{
    if (!pid->initialized) return 0.0f;
    
    float error = pid->setpoint - input;
    
    pid->integral += error;
    if (pid->integral > pid->integral_limit_max) {
        pid->integral = pid->integral_limit_max;
    } else if (pid->integral < pid->integral_limit_min) {
        pid->integral = pid->integral_limit_min;
    }
    
    float derivative = error - pid->previous_error;
    pid->previous_error = error;
    
    float output = (pid->kp * error) + (pid->ki * pid->integral) + (pid->kd * derivative);
    
    if (output > pid->output_limit_max) {
        output = pid->output_limit_max;
    } else if (output < pid->output_limit_min) {
        output = pid->output_limit_min;
    }
    
    return output;
}

void pid_reset(pid_controller_t *pid)
{
    pid->integral = 0.0f;
    pid->previous_error = 0.0f;
}