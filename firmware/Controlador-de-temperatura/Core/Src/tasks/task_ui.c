#include "tasks/task_ui.h"
#include "hardware/adc_hw.h"
#include "hardware/oled_hw.h"
#include "control/pid_controller.h"
#include "main.h"

extern float get_setpoint_callback(void *);
extern float get_kp_callback(void *);
extern float get_ki_callback(void *);
extern float get_kd_callback(void *);

void task_ui(void)
{
    static uint32_t update_counter = 0;
    static float temp = 20.0;

    update_counter++;
    if (update_counter % 10 == 0)
    {
        oled_hw_clear();

        temp = 0.3 * adc_hw_read_temperature() + 0.7 * temp;

        oled_hw_print_str("Temp = ", 0);
        oled_hw_print_float_at(temp, 0, 32);
        oled_hw_print_str("SetP = ", 2);
        oled_hw_print_float_at(pid_get_setpoint(get_temp_pid_instance()), 2, 32);
        oled_hw_print_str_at("P:", 4, 0);
        oled_hw_print_float_at(pid_get_kp(get_temp_pid_instance()), 4, 20);
        oled_hw_print_str_at("I:", 4, 72);
        oled_hw_print_float_at(pid_get_ki(get_temp_pid_instance()), 4, 92);
        oled_hw_print_str_at("D:", 6, 0);
        oled_hw_print_float_at(pid_get_kd(get_temp_pid_instance()), 6, 20);
        oled_hw_update();
        update_counter = 0;
    }
}