#include "communication/comm_message_buffer.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include "services/error_handler.h"
#include "main.h"
#include <string.h>

static msg_message_t msg_buffers[COMM_IFACE_MAX];

void msg_buffer_init(void)
{
    for (int i = 0; i < COMM_IFACE_MAX; i++)
    {
        msg_buffers[i].state = MSG_STATE_WAITING_DELIMITER;
        msg_buffers[i].len = 0;
    }
}

void msg_extract_from_rx(comm_interface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return;

    msg_message_t *msg = &msg_buffers[iface_id];

    if (msg->state == MSG_STATE_ERROR)
    {
        msg->state = MSG_STATE_WAITING_DELIMITER;
        msg->len = 0;
    }

    if (msg->state == MSG_STATE_READY)
    {
        return;
    }

    size_t available = comm_buffer_rx_count(iface_id);
    if (available == 0)
    {
        return;
    }
    
    uint8_t byte;
    size_t to_read = available;

    while (to_read > 0)
    {
        size_t read_len = 1;
        if (!comm_buffer_rx_get(iface_id, &byte, &read_len))
            break;

        to_read--;

        if (msg->len < MESSAGE_BUFFER_SIZE - 1)
        {
            if (byte == '\r')
            {
                NULL; // se ignoran los \r para compativilidad con el standar SCPI
            }
            else
            {
                msg->data[msg->len++] = byte;
                if (byte == '\n')
                {
                    if (msg->len > 1)
                    {
                        msg->state = MSG_STATE_READY;
                        return;
                    }
                }
            }
        }
        else
        {
            msg->state = MSG_STATE_ERROR;
            comm_buffer_rx_clear(iface_id);
            msg->len = 0;
            return;
        }
    }
    HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
}

const char *msg_get_next(comm_interface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return NULL;

    msg_message_t *msg = &msg_buffers[iface_id];

    if (msg->state == MSG_STATE_READY)
    {
        msg->data[msg->len] = '\0';
        return (const char *)msg->data;
    }

    return NULL;
}

void msg_mark_processed(comm_interface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return;

    msg_message_t *msg = &msg_buffers[iface_id];

    if (msg->state == MSG_STATE_READY)
    {
        msg->state = MSG_STATE_WAITING_DELIMITER;
        msg->len = 0;
    }
}

size_t msg_get_len(comm_interface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return 0;
    return msg_buffers[iface_id].len;
}

bool msg_contains_query(comm_interface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return false;

    msg_message_t *msg = &msg_buffers[iface_id];

    if (msg->state != MSG_STATE_READY)
        return false;

    for (size_t i = 0; i < msg->len; i++)
    {
        if (msg->data[i] == '?')
            return true;
    }

    return false;
}

bool msg_is_ready(comm_interface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return false;

    return msg_buffers[iface_id].state == MSG_STATE_READY;
}