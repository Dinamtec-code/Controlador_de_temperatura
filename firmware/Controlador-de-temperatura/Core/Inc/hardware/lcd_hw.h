#ifndef LCD_HW_H
#define LCD_HW_H

#include <stdint.h>

#define LCD_I2C_ADDRESS 0x78

void lcd_hw_init(void);
void lcd_hw_display(float temp, float setpoint, float kp, float kd);

#endif