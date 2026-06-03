#include "hardware/hrtim_hw.h"
#include "hrtim.h"
#include <math.h>

#define HRTIM_PERIOD 64000

static HRTIM_CompareCfgTypeDef sCompareCfg = {0};

void hrtim_hw_init(void)
{
}

void hrtim_hw_set_duty(hrtim_output_t output, float duty)
{
    uint32_t compare_value = (uint32_t)(duty * HRTIM_PERIOD / 100.0f);
    
    if (compare_value > HRTIM_PERIOD) {
        compare_value = HRTIM_PERIOD;
    }
    
    sCompareCfg.CompareValue = compare_value;
    
    switch (output) {
        case HRTIM_OUTPUT_CH_TA1:
            HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_A, HRTIM_COMPAREUNIT_1, &sCompareCfg);
            break;
        case HRTIM_OUTPUT_CH_TB1:
            HAL_HRTIM_WaveformCompareConfig(&hhrtim1, HRTIM_TIMERINDEX_TIMER_B, HRTIM_COMPAREUNIT_1, &sCompareCfg);
            break;
    }
}

void hrtim_hw_start(void)
{
    HAL_HRTIM_WaveformOutputStart(&hhrtim1, HRTIM_OUTPUT_TA1 | HRTIM_OUTPUT_TB1);
}