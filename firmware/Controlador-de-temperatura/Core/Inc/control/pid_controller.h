#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#define ANTI_WINDUP 1
#define PID_CHANNELS 2

#include "arm_math_types.h"
#include "control/controller_functions.h"
#include "stdbool.h"
#include "stdint.h"

#define PID_OUTPUT_CHANNELS 2

typedef struct
{
    arm_pid_f32_t cmsis_pid;
    float output_limit_min;
    float output_limit_max;
    uint8_t initialized;
    float setpoint;
    bool output_state[PID_OUTPUT_CHANNELS];
} pid_controller_t;

void pid_init(pid_controller_t *pid, float kp, float ki, float kd,float out_min, float out_max, float setpoint);
void pid_set_parameters(pid_controller_t *pid, float kp, float ki, float kd);
void pid_set_setpoint(pid_controller_t *pid, float setpoint);
void pid_set_limits(pid_controller_t *pid, float out_min, float out_max);
float pid_get_limit_min(pid_controller_t *pid);
float pid_get_limit_max(pid_controller_t *pid);
float pid_compute(pid_controller_t *pid, float input);
void pid_reset(pid_controller_t *pid);

void pid_set_kp(pid_controller_t *pid, float kp);
void pid_set_ki(pid_controller_t *pid, float ki);
void pid_set_kd(pid_controller_t *pid, float kd);
pid_controller_t *get_pid_instance(uint8_t channel);
float pid_get_kp(pid_controller_t *pid);
float pid_get_ki(pid_controller_t *pid);
float pid_get_kd(pid_controller_t *pid);
float pid_get_setpoint(pid_controller_t *pid);
float pid_get_out(pid_controller_t *pid);
bool pid_controller_get_output(pid_controller_t *pid, int channel);
void pid_controller_set_output(pid_controller_t *pid, int channel, bool value);

#endif