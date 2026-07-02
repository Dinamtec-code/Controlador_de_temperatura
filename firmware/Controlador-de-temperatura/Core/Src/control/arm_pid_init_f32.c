#include "control/controller_functions.h"
#include <stdint.h>
#include <string.h>
#include "tasks/scheduler.h"

void arm_pid_init_f32(arm_pid_f32_t *S, int32_t resetStateFlag)
{
    S->A0 = S->Kp + S->Ki * ((float)TASK_CONTROL_PERIOD_MS) / 60000.0f + S->Kd;
    S->A1 = (-S->Kp) - (2.0f * S->Kd);
    S->A2 = S->Kd;
    if (resetStateFlag)
    {
        memset(S->state, 0, 3U * sizeof(float32_t));
    }
}

void arm_pid_reset_f32(arm_pid_f32_t *S)
{
    memset(S->state, 0, 3U * sizeof(float32_t));
}