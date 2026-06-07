#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    COMM_IFACE_USART = 0,
    COMM_IFACE_TCP,
    COMM_IFACE_USB,
    COMM_IFACE_MAX
} comm_interface_id_t;

typedef struct comm_interface comm_interface_t;

typedef void (*comm_rx_indication_callback_t)(comm_interface_id_t iface_id, size_t len);
typedef void (*comm_tx_complete_callback_t)(comm_interface_id_t iface_id);

struct comm_interface {
    comm_interface_id_t id;
    void *context;
    const char *name;

    bool (*send)(const uint8_t *data, size_t len);
    bool (*is_connected)(void *context);

    comm_rx_indication_callback_t rx_indication_cb;
    comm_tx_complete_callback_t tx_complete_cb;
};

void comm_register_interface(comm_interface_t *iface);
void comm_unregister_interface(comm_interface_t *iface);
comm_interface_t *comm_get_interface(comm_interface_id_t id);

#endif