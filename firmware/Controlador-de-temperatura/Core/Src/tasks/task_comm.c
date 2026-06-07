#include "tasks/task_comm.h"
#include "hardware/oled_hw.h"
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
        static scpi_output_interface_t out_iface = {
            .send_response = send_response_to_interface,
            .context = NULL};
        scpi_set_output_interface(&out_iface);
        output_iface_set = true;
    }

    comm_interface_t *iface = comm_get_interface(COMM_IFACE_USART);

    if (!iface || !comm_interface_is_rx_active(COMM_IFACE_USART))
    {
        comm_interface_start_rx(COMM_IFACE_USART);
        comm_interface_set_rx_active(COMM_IFACE_USART, true);
        return;
    }

    if (comm_interface_has_error(COMM_IFACE_USART))
    {
        comm_interface_reset(COMM_IFACE_USART);
        return;
    }

    process_rx_data();

    if (comm_buffer_tx_count(COMM_IFACE_USART) > 0)
    {
        comm_interface_set_tx_busy(COMM_IFACE_USART, true);
    }

    if (comm_interface_is_tx_busy(COMM_IFACE_USART) && comm_interface_is_tx_ready(COMM_IFACE_USART))
    {
        if (!comm_interface_start_tx(COMM_IFACE_USART))
        {
            comm_interface_set_tx_busy(COMM_IFACE_USART, false);
        }
    }

    if (error_check(ERROR_REMOTE_RX_OVERFLOW))
    {
        const char *msg = "ERR:RX_BUFFER_OVERFLOW\r\n";
        comm_interface_send(COMM_IFACE_USART, (const uint8_t *)msg, strlen(msg));
        error_clear(ERROR_REMOTE_RX_OVERFLOW);
    }

    if (error_check(ERROR_TX_BUFFER_FULL))
    {
        const char *msg = "ERR:TX_BUFFER_FULL\r\n";
        comm_interface_send(COMM_IFACE_USART, (const uint8_t *)msg, strlen(msg));
        error_clear(ERROR_TX_BUFFER_FULL);
    }
}