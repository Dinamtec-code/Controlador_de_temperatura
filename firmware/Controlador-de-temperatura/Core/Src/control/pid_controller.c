#include "control/pid_controller.h"
#include "stdbool.h"

static pid_controller_t temp_pid;

pid_controller_t *get_temp_pid_instance(void)
{
    return &temp_pid;
}

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
    pid->output_limit_min = -95.0f;
    pid->output_limit_max = 95.0f;
    pid->initialized = 1;
    pid->output_state[0] = false;
    pid->output_state[1] = false;
    pid->setpoint = 20.0f;
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

float pid_get_limit_min(pid_controller_t *pid)
{
    return pid->output_limit_min;
}

float pid_get_limit_max(pid_controller_t *pid)
{
    return pid->output_limit_max;
}

float pid_compute(pid_controller_t *pid, float input)
{
    if (!pid->initialized)
        return 0.0f;

    float error = pid->setpoint - input;
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

void pid_set_setpoint(pid_controller_t *pid, float setpoint)
{
    pid->setpoint = setpoint;
}

void pid_set_kp(pid_controller_t *pid, float kp)
{
    pid->cmsis_pid.Kp = kp;
    arm_pid_init_f32(&pid->cmsis_pid, 0);
}
void pid_set_ki(pid_controller_t *pid, float ki)
{
    pid->cmsis_pid.Ki = ki;
    arm_pid_init_f32(&pid->cmsis_pid, 0);
}
void pid_set_kd(pid_controller_t *pid, float kd)
{
    pid->cmsis_pid.Kd = kd;
    arm_pid_init_f32(&pid->cmsis_pid, 0);
}
float pid_get_setpoint(pid_controller_t *pid)
{
    return pid->setpoint;
}
float pid_get_kp(pid_controller_t *pid)
{
    return pid->cmsis_pid.Kp;
}
float pid_get_ki(pid_controller_t *pid)
{
    return pid->cmsis_pid.Ki;
}
float pid_get_kd(pid_controller_t *pid)
{
    return pid->cmsis_pid.Kd;
}
float pid_get_out(pid_controller_t *pid)
{
    return pid->cmsis_pid.state[2];
}
bool pid_controller_get_output(pid_controller_t *pid, int channel)
{
    return pid->output_state[channel];
}
void pid_controller_set_output(pid_controller_t *pid, int channel, bool value)
{
    pid->output_state[channel] = value;
}
