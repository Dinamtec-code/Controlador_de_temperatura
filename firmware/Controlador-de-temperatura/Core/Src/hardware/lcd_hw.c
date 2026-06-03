#include "hardware/lcd_hw.h"
#include "stm32f3xx_hal_i2c.h"
#include <string.h>
#include <stdio.h>

extern I2C_HandleTypeDef hi2c1;

static void lcd_send_cmd(char cmd)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    data_t[0] = data_u | 0x0C;
    data_t[1] = data_u | 0x08;
    data_t[2] = data_l | 0x0C;
    data_t[3] = data_l | 0x08;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDRESS << 1, data_t, 4, 100);
}

static void lcd_send_data(char data)
{
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    data_t[0] = data_u | 0x0D;
    data_t[1] = data_u | 0x09;
    data_t[2] = data_l | 0x0D;
    data_t[3] = data_l | 0x09;
    HAL_I2C_Master_Transmit(&hi2c1, LCD_I2C_ADDRESS << 1, data_t, 4, 100);
}

void lcd_hw_init(void)
{
    HAL_Delay(50);
    lcd_send_cmd(0x20);
    HAL_Delay(1);
    lcd_send_cmd(0x20);
    HAL_Delay(1);
    lcd_send_cmd(0x80);
    HAL_Delay(1);
    lcd_send_cmd(0x00);
    HAL_Delay(1);
    lcd_send_cmd(0xC0);
    HAL_Delay(1);
    lcd_send_cmd(0x00);
    HAL_Delay(1);
    lcd_send_cmd(0x60);
    HAL_Delay(1);
}

void lcd_hw_display(float temp, float setpoint, float kp, float kd)
{
    char buf[17];
    memset(buf, ' ', 16);
    buf[16] = '\0';

    snprintf(buf, 17, "T:%.1f S:%.1f", temp, setpoint);
    for (int i = 0; i < 16; i++) {
        lcd_send_data(buf[i]);
    }
    lcd_send_cmd(0xC0);
    snprintf(buf, 17, "KP:%.2f KD:%.2f", kp, kd);
    for (int i = 0; i < 16; i++) {
        lcd_send_data(buf[i]);
    }
}