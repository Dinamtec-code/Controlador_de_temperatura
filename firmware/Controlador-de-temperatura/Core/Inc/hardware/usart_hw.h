#ifndef USART_HW_H
#define USART_HW_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "hardware/circular_buffer.h"

#define USART_HW_RX_BUFFER_SIZE 128

extern UART_HandleTypeDef huart2;
extern circular_buffer_t rx_circular_buffer;

void usart_hw_init(void);
void usart_hw_start_rx(void);
void usart_hw_idle_handler(void);
void usart_hw_send_str(const char *str);
void usart_hw_send_buf(uint8_t *data, size_t len);

#endif