#ifndef CONTROLLER_FUNCTIONS_H_
#define CONTROLLER_FUNCTIONS_H_

#include "arm_math_types.h"
#include "arm_math_memory.h"

#ifndef __STATIC_FORCEINLINE
#define __STATIC_FORCEINLINE static inline
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct arm_pid_instance_f32 arm_pid_f32_t;

    struct arm_pid_instance_f32
    {
        float32_t A0;
        float32_t A1;
        float32_t A2;
        float32_t state[3];
        float32_t Kp;
        float32_t Ki;
        float32_t Kd;
    };

    void arm_pid_init_f32(arm_pid_f32_t *S, int32_t resetStateFlag);
    void arm_pid_reset_f32(arm_pid_f32_t *S);
    __STATIC_FORCEINLINE float32_t arm_pid_f32(arm_pid_f32_t *S, float32_t in)
    {
        float32_t out;
        out = (S->A0 * in) + (S->A1 * S->state[0]) + (S->A2 * S->state[1]) + (S->state[2]);
        S->state[1] = S->state[0];
        S->state[0] = in;
        S->state[2] = out;
        return (out);
    }

    void arm_pid_reset_f32(arm_pid_f32_t *S);

#ifdef __cplusplus
}
#endif

#endif