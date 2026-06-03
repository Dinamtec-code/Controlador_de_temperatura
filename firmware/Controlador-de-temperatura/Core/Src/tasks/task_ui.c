#include "tasks/task_ui.h"
#include "hardware/adc_hw.h"
#include "hardware/oled_hw.h"
#include "main.h"

static uint32_t update_counter = 0;

extern float temperatureSetpoint;
extern float pidKp, pidKi, pidKd;

void task_ui(void)
{
    update_counter++;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    if (update_counter >= 50) {
        float temp = adc_hw_read_temperature();
        oled_hw_clear();
        oled_hw_print_float(temp, 2);
        oled_hw_update();
        update_counter = 0;
    }
}