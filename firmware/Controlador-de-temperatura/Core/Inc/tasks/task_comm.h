#ifndef TASK_COMM_H
#define TASK_COMM_H

#define RX_BUFFER_SIZE 480U
#define TX_BUFFER_SIZE 480U

void comm_task_get_buffers(cb_t **rx_buff, cb_t **tx_buff);
void comm_task_set_active_iface(comm_iface_t *iface);

void task_comm(void);

void fsm_gestion(void);
void fsm_rx(void);
void fsm_tx(void);

#endif