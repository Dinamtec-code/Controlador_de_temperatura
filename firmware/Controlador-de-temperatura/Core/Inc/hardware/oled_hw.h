#ifndef OLED_HW_H
#define OLED_HW_H

#include <stdint.h>

#define OLED_I2C_ADDRESS 0x3D

void oled_hw_init(void);
void oled_hw_clear(void);
void oled_hw_update(void);
void oled_hw_print_num(int num, uint8_t page);
void oled_hw_print_float(float val, uint8_t page);

#endif