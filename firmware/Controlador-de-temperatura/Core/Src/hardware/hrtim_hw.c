#include "hardware/hrtim_hw.h"
#include "hrtim.h"
#include <math.h>

#define HRTIM_PERIOD 64000

extern HRTIM_HandleTypeDef hhrtim1;

void hrtim_hw_init(void)
{
}

void hrtim_hw_set_duty(hrtim_output_t output, float duty)
{
    uint32_t compare_value = (uint32_t)((100 - duty) * HRTIM_PERIOD / 100.0f);

    if (compare_value > HRTIM_PERIOD)
    {
        compare_value = HRTIM_PERIOD;
    }
    else if (compare_value < 500)
    {
        compare_value = 500;
    }

    switch (output)
    {
    case HRTIM_OUTPUT_CH_TA1:
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, compare_value);
        break;
    case HRTIM_OUTPUT_CH_TB1:
        __HAL_HRTIM_SETCOMPARE(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1, compare_value);
        break;
    }
}

void hrtim_hw_start(void)
{
    //    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TB1);
    HAL_HRTIM_SimpleBaseStart(&hhrtim1, HRTIM_TIMERINDEX_MASTER);
    HAL_HRTIM_SimplePWMStart(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1);
    HAL_HRTIM_SimplePWMStart(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA2);
    HAL_HRTIM_SimplePWMStart(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1);
    HAL_HRTIM_SimplePWMStart(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB2);
}

void hrtim_hw_stop(void)
{
    HAL_HRTIM_SimpleBaseStop(&hhrtim1, HRTIM_TIMERINDEX_MASTER);
    HAL_HRTIM_SimplePWMStop(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA1);
    HAL_HRTIM_SimplePWMStop(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_OUTPUT_TA2);
    HAL_HRTIM_SimplePWMStop(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB1);
    HAL_HRTIM_SimplePWMStop(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_OUTPUT_TB2);
}