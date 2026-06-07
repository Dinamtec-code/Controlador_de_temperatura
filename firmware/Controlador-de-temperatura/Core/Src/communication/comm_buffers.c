#include "communication/comm_buffers.h"
#include <string.h>

static circular_buffer_t rx_buffers[COMM_IFACE_MAX];
static uint8_t rx_buffer_mem[COMM_IFACE_MAX][COMM_BUFFER_RX_SIZE];
static circular_buffer_t tx_buffers[COMM_IFACE_MAX];
static uint8_t tx_buffer_mem[COMM_IFACE_MAX][COMM_BUFFER_TX_SIZE];

static comm_interface_t *registered_interfaces[COMM_IFACE_MAX];

void comm_buffers_init(void) {
    for (int i = 0; i < COMM_IFACE_MAX; i++) {
        cb_init(&rx_buffers[i], rx_buffer_mem[i], COMM_BUFFER_RX_SIZE);
        cb_init(&tx_buffers[i], tx_buffer_mem[i], COMM_BUFFER_TX_SIZE);
        registered_interfaces[i] = NULL;
    }
}

bool comm_buffer_rx_put(comm_interface_id_t iface_id, uint8_t data) {
    if (iface_id >= COMM_IFACE_MAX) return false;
    return cb_put(&rx_buffers[iface_id], data) == BUF_OK;
}

bool comm_buffer_tx_put(comm_interface_id_t iface_id, const uint8_t *data, size_t len) {
    if (iface_id >= COMM_IFACE_MAX || len == 0) return false;
    
    for (size_t i = 0; i < len; i++) {
        if (cb_put(&tx_buffers[iface_id], data[i]) != BUF_OK) {
            return false;
        }
    }
    return true;
}

size_t comm_buffer_rx_count(comm_interface_id_t iface_id) {
    if (iface_id >= COMM_IFACE_MAX) return 0;
    return cb_count(&rx_buffers[iface_id]);
}

bool comm_buffer_rx_get(comm_interface_id_t iface_id, uint8_t *data, size_t *len) {
    if (iface_id >= COMM_IFACE_MAX || !data || *len == 0) return false;
    
    size_t available = cb_count(&rx_buffers[iface_id]);
    size_t to_read = (available < *len) ? available : *len;
    
    for (size_t i = 0; i < to_read; i++) {
        if (cb_get(&rx_buffers[iface_id], &data[i]) != BUF_OK) {
            return false;
        }
    }
    *len = to_read;
    return true;
}

void comm_register_interface(comm_interface_t *iface) {
    if (!iface || iface->id >= COMM_IFACE_MAX) return;
    registered_interfaces[iface->id] = iface;
}

void comm_unregister_interface(comm_interface_t *iface) {
    if (!iface || iface->id >= COMM_IFACE_MAX) return;
    registered_interfaces[iface->id] = NULL;
}

comm_interface_t *comm_get_interface(comm_interface_id_t id) {
    if (id >= COMM_IFACE_MAX) return NULL;
    return registered_interfaces[id];
}

bool comm_buffer_tx_get(comm_interface_id_t iface_id, uint8_t *data, size_t *len) {
    if (iface_id >= COMM_IFACE_MAX || !data || *len == 0) return false;
    
    size_t available = cb_count(&tx_buffers[iface_id]);
    size_t to_read = (available < *len) ? available : *len;
    
    for (size_t i = 0; i < to_read; i++) {
        if (cb_get(&tx_buffers[iface_id], &data[i]) != BUF_OK) {
            return false;
        }
    }
    *len = to_read;
    return true;
}