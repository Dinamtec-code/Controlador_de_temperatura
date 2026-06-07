#include "tasks/task_comm.h"
#include "communication/comm_interface.h"
#include "services/scpi_parser.h"
#include "communication/comm_buffers.h"
#include "services/error_handler.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

static char cmd_buffer[128];
static size_t cmd_len = 0;

static void send_response_to_interface(const char *resp, void *context)
{
    comm_buffer_tx_put(COMM_IFACE_USART, (const uint8_t *)resp, strlen(resp));
}

static void process_rx_data(void)
{
    uint8_t byte;
    size_t available = comm_buffer_rx_count(COMM_IFACE_USART);

    for (size_t i = 0; i < available && cmd_len < sizeof(cmd_buffer) - 1; i++)
    {
        size_t read_len = 1;
        if (comm_buffer_rx_get(COMM_IFACE_USART, &byte, &read_len) != true)
        {
            continue;
        }

        if (byte == '\n' || byte == '\r' || byte == '\0')
        {
            cmd_buffer[cmd_len] = '\0';
            if (cmd_len > 0)
            {
                scpi_process_line(cmd_buffer);
            }
            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            cmd_len = 0;
        }
        else
        {
            cmd_buffer[cmd_len++] = (char)byte;
        }
    }
}

void task_comm(void)
{
    static bool output_iface_set = false;

    if (!output_iface_set)
    {
        scpi_output_interface_t out_iface = {
            .send_response = send_response_to_interface,
            .context = NULL};
        scpi_set_output_interface(&out_iface);
        output_iface_set = true;
    }

    comm_interface_state_t state = comm_interface_get_state(COMM_IFACE_USART);

    if (state == COMM_STATE_UNINIT)
    {
        comm_interface_start_rx(COMM_IFACE_USART);
        return;
    }

    if (state == COMM_STATE_ERROR)
    {
        comm_interface_reset(COMM_IFACE_USART);
        return;
    }

    if (state == COMM_STATE_RX_ACTIVE)
    {
        process_rx_data();
        if (comm_buffer_tx_count(COMM_IFACE_USART) > 0)
        {
            comm_interface_set_state(COMM_IFACE_USART, COMM_STATE_TX_BUSY);
        }
    }

    if (state == COMM_STATE_TX_BUSY && comm_interface_is_tx_ready(COMM_IFACE_USART))
    {
        if (!comm_interface_start_tx(COMM_IFACE_USART))
        {
            comm_interface_set_state(COMM_IFACE_USART, COMM_STATE_RX_ACTIVE);
        }
    }

    if (error_check(ERROR_REMOTE_RX_OVERFLOW))
    {
        const char *msg = "ERR:RX_BUFFER_OVERFLOW\r\n";
        comm_interface_send(COMM_IFACE_USART, (const uint8_t *)msg, strlen(msg));
        error_clear(ERROR_REMOTE_RX_OVERFLOW);
    }
}