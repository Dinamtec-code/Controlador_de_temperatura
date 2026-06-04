#include "control/pid_controller.h"

void pid_init(pid_controller_t *pid, float kp, float ki, float kd)
{
    pid->cmsis_pid.A0 = 0.0f;
    pid->cmsis_pid.A1 = 0.0f;
    pid->cmsis_pid.A2 = 0.0f;
    pid->cmsis_pid.state[0] = 0.0f;
    pid->cmsis_pid.state[1] = 0.0f;
    pid->cmsis_pid.state[2] = 0.0f;
    pid->cmsis_pid.Kp = kp;
    pid->cmsis_pid.Ki = ki;
    pid->cmsis_pid.Kd = kd;
    pid->output_limit_min = -100.0f;
    pid->output_limit_max = 100.0f;
    pid->initialized = 1;
    arm_pid_init_f32(&pid->cmsis_pid, 1);
}

void pid_set_parameters(pid_controller_t *pid, float kp, float ki, float kd)
{
    pid->cmsis_pid.Kp = kp;
    pid->cmsis_pid.Ki = ki;
    pid->cmsis_pid.Kd = kd;
    arm_pid_init_f32(&pid->cmsis_pid, 0);
}

void pid_set_limits(pid_controller_t *pid, float out_min, float out_max)
{
    pid->output_limit_min = out_min;
    pid->output_limit_max = out_max;
}

float pid_compute(pid_controller_t *pid, float setpoint, float input)
{
    if (!pid->initialized)
        return 0.0f;

    float error = setpoint - input;
    float output = arm_pid_f32(&pid->cmsis_pid, error);

    if (output > pid->output_limit_max)
    {
        output = pid->output_limit_max;
#if ANTI_WINDUP
        pid->cmsis_pid.state[2] = output;
#endif
    }
    else if (output < pid->output_limit_min)
    {
        output = pid->output_limit_min;
#if ANTI_WINDUP
        pid->cmsis_pid.state[2] = output;
#endif
    }

    return output;
}

void pid_reset(pid_controller_t *pid)
{
    arm_pid_reset_f32(&pid->cmsis_pid);
}