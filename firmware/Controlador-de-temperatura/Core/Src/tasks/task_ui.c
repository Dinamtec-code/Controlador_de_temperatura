#include "tasks/task_ui.h"
#include "hardware/adc_hw.h"
#include "hardware/oled_hw.h"
#include "main.h"

void task_ui(void)
{
    static uint32_t update_counter = 0;

    update_counter++;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    if (update_counter >= 50) {
        oled_hw_test_pattern();
        update_counter = 0;
    }
}