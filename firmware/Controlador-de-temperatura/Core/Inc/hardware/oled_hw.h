#ifndef OLED_HW_H
#define OLED_HW_H

#include <stdint.h>

#define OLED_I2C_ADDRESS 0x78

void oled_hw_init(void);
void oled_hw_clear(void);
void oled_hw_update(void);
void oled_hw_test_pattern(void);
void oled_hw_print_num(int num, uint8_t page);
void oled_hw_print_float(float val, uint8_t page);
void oled_hw_print_float_at(float val, uint8_t page, uint8_t x_offset);
void oled_hw_print_str(const char *str, uint8_t page);
void oled_hw_print_str_at(const char *str, uint8_t page, uint8_t offset);

#endif