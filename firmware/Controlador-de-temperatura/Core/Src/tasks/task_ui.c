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

        temp = adc_hw_get_temp_diff();

        oled_hw_print_str("Temp = ", 1);
        oled_hw_print_float_at(temp, 1, 32);
        oled_hw_print_str("SetP = ", 3);
        oled_hw_print_float_at(pid_get_setpoint(get_temp_pid_instance()), 3, 32);
        oled_hw_print_str_at("P:", 5, 0);
        oled_hw_print_float_at(pid_get_kp(get_temp_pid_instance()), 5, 20);
        oled_hw_print_str_at("I:", 5, 72);
        oled_hw_print_float_at(pid_get_ki(get_temp_pid_instance()), 5, 92);
        oled_hw_print_str_at("D:", 7, 0);
        oled_hw_print_float_at(pid_get_kd(get_temp_pid_instance()), 7, 20);
        oled_hw_print_str_at("O:", 7, 72);
        oled_hw_print_float_at(pid_get_out(get_temp_pid_instance()), 7, 92);
        oled_hw_update();
        update_counter = 0;
    }
}