#ifndef TASK_COMM_H
#define TASK_COMM_H

#define RX_BUFFER_SIZE 480U
#define TX_BUFFER_SIZE 480U

void task_comm(void);

void fsm_gestion(void);
void fsm_rx(void);
void fsm_tx(void);

#endif