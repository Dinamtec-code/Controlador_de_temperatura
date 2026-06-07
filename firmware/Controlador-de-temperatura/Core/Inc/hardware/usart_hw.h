#ifndef USART_HW_H
#define USART_HW_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

extern UART_HandleTypeDef huart2;

void usart_hw_init(void);
void usart_hw_start_rx(void);
void usart_hw_idle_handler(void);
void usart_hw_send_str(const char *str);
void usart_hw_send_buf(uint8_t *data, size_t len);
bool usart_hw_is_tx_ready(void);
bool usart_hw_transmit_from_system_buffer(void);

#endif