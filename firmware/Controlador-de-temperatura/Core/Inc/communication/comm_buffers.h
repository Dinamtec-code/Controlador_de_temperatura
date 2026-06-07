#ifndef COMM_BUFFERS_H
#define COMM_BUFFERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "services/circular_buffer.h"
#include "communication/comm_interface.h"

#define COMM_BUFFER_RX_SIZE 256
#define COMM_BUFFER_TX_SIZE 256

typedef struct {
    comm_interface_id_t iface_id;
    size_t len;
} comm_rx_message_t;

typedef struct {
    bool (*read_msg)(comm_rx_message_t *msg);
    size_t (*get_pending_tx_len)(comm_interface_id_t iface_id);
} comm_buffers_api_t;

void comm_buffers_init(void);
bool comm_buffer_rx_put(comm_interface_id_t iface_id, uint8_t data);
bool comm_buffer_tx_put(comm_interface_id_t iface_id, const uint8_t *data, size_t len);
bool comm_buffer_rx_get(comm_interface_id_t iface_id, uint8_t *data, size_t *len);
bool comm_buffer_tx_get(comm_interface_id_t iface_id, uint8_t *data, size_t *len);
size_t comm_buffer_rx_count(comm_interface_id_t iface_id);
void comm_buffer_rx_clear(comm_interface_id_t iface_id);

#endif