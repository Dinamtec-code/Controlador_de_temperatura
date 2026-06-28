#ifndef COMM_MESSAGE_BUFFER_H
#define COMM_MESSAGE_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "communication/comm_driver_api.h"

#define MESSAGE_BUFFER_SIZE 256

typedef enum {
    MSG_STATE_WAITING_DELIMITER,
    MSG_STATE_RECEIVING,
    MSG_STATE_READY,
    MSG_STATE_ERROR
} msg_state_t;

typedef struct {
    msg_state_t state;
    uint8_t data[MESSAGE_BUFFER_SIZE];
    size_t len;
} msg_message_t;

void msg_buffer_init(void);

void msg_extract_from_rx(comm_iface_t iface_id);

const char *msg_get_next(comm_iface_t iface_id);

void msg_mark_processed(comm_iface_t iface_id);

bool msg_contains_query(comm_iface_t iface_id);

bool msg_is_ready(comm_iface_t iface_id);

#endif