#ifndef TASK_COMM_H
#define TASK_COMM_H

#include "services/circular_buffer.h"
#include "communication/comm_driver_api.h"
#include "communication/app_msg_api.h"

#define RX_BUFFER_SIZE 480U
#define TX_BUFFER_SIZE 480U

void comm_task_get_buffers(cb_t **rx_buff, cb_t **tx_buff);
void comm_task_set_active_iface(comm_iface_t *iface);

void comm_task_register_app_iface(const app_msg_iface_t *iface);
void comm_app_send_response(const uint8_t *data, size_t len);

void task_comm(void);

void fsm_gestion(void);
void fsm_rx(void);
void fsm_tx(void);

#endif