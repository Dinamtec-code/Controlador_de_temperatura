#include "tasks/task_ui.h"
#include "hardware/adc_hw.h"
#include "hardware/oled_hw.h"
#include "main.h"

void task_ui(void)
{
    static uint32_t update_counter = 0;
    static float temp = 20.0;

    update_counter++;
    if (update_counter % 10 == 0)
    {
        oled_hw_clear();

        temp = 0.3 * adc_hw_read_temperature() + 0.7 * temp;

        oled_hw_print_float(temp, 2);
        oled_hw_update();
        update_counter = 0;
    }
}