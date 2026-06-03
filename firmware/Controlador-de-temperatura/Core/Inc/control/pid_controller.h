#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <stdint.h>

typedef struct {
    float kp;
    float ki;
    float kd;
    float setpoint;
    float integral;
    float previous_error;
    float output_limit_min;
    float output_limit_max;
    float integral_limit_min;
    float integral_limit_max;
    uint8_t initialized;
} pid_controller_t;

void pid_init(pid_controller_t *pid, float kp, float ki, float kd);
void pid_set_parameters(pid_controller_t *pid, float kp, float ki, float kd);
void pid_set_limits(pid_controller_t *pid, float out_min, float out_max, float int_min, float int_max);
void pid_set_setpoint(pid_controller_t *pid, float setpoint);
float pid_compute(pid_controller_t *pid, float input);
void pid_reset(pid_controller_t *pid);

#endif