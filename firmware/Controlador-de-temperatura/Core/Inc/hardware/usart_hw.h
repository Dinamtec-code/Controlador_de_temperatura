#ifndef USART_HW_H
#define USART_HW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "main.h"
#include "services/circular_buffer.h"
#include "communication/comm_driver_api.h"

extern UART_HandleTypeDef huart2;

/* --- Registro con inyección simétrica --- */
void usart_iface_register(circular_buffer_t *rx_cb, circular_buffer_t *tx_cb);

comm_response_t usart_hw_start_rx(void *context);
comm_response_t usart_hw_stop_rx(void *context);
comm_response_t usart_hw_start_tx(void *context);

comm_iface_event_t usart_hw_get_event(void *ctx);

#endif /* USART_HW_H */