#include "tasks/task_ui.h"
#include "hardware/adc_hw.h"
#include "hardware/lcd_hw.h"

static uint32_t update_counter = 0;

extern float temperatureSetpoint;
extern float pidKp, pidKi, pidKd;

void task_ui(void)
{
    update_counter++;
    if (update_counter >= 50) {
        float temp = adc_hw_read_temperature();
        lcd_hw_display(temp, temperatureSetpoint, pidKp, pidKd);
        update_counter = 0;
    }
}