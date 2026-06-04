#include "control/controller_functions.h"
#include <stdint.h>
#include <string.h>

void arm_pid_init_f32(arm_pid_instance_f32 *S, int32_t resetStateFlag)
{
    S->A0 = S->Kp + S->Ki + S->Kd;
    S->A1 = (-S->Kp) - (2.0f * S->Kd);
    S->A2 = S->Kd;
    if (resetStateFlag)
    {
        memset(S->state, 0, 3U * sizeof(float32_t));
    }
}

void arm_pid_reset_f32(arm_pid_instance_f32 *S)
{
    memset(S->state, 0, 3U * sizeof(float32_t));
}