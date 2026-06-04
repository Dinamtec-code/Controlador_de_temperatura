#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#define ANTI_WINDUP 1

#include "arm_math_types.h"
#include "control/controller_functions.h"

typedef struct
{
    arm_pid_instance_f32 cmsis_pid;
    float output_limit_min;
    float output_limit_max;
    uint8_t initialized;
    float prev_output;
} pid_controller_t;

void pid_init(pid_controller_t *pid, float kp, float ki, float kd);
void pid_set_parameters(pid_controller_t *pid, float kp, float ki, float kd);
void pid_set_limits(pid_controller_t *pid, float out_min, float out_max);
float pid_compute(pid_controller_t *pid, float setpoint, float input);
void pid_reset(pid_controller_t *pid);

#endif