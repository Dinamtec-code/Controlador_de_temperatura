#ifndef I2C_HW_H
#define I2C_HW_H

#include <stdint.h>
#include "main.h"

#define I2C_HW_LCD_ADDRESS 0x78

void i2c_hw_init(void);
void i2c_hw_write(uint8_t addr, uint8_t *data, uint16_t len);

#endif