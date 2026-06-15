#include "tasks/task_comm.h"
#include "hardware/oled_hw.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include "communication/comm_message_buffer.h"
#include "services/scpi_parser.h"
#include "services/error_handler.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

static volatile bool msg_ready_flag = false;

static void send_response_to_interface(const char *resp, void *context)
{
    comm_buffer_tx_put(COMM_IFACE_USART, (const uint8_t *)resp, strlen(resp));
}

void task_comm(void)
{
    static bool output_iface_set = false;
    static bool prev_msg_ready = false;

    if (!output_iface_set)
    {
        static scpi_output_interface_t out_iface = {
            .send_response = send_response_to_interface,
            .context = NULL};
        scpi_set_output_interface(&out_iface);
        msg_buffer_init();
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
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
        HAL_Delay(100);
        comm_interface_reset(COMM_IFACE_USART);
        return;
    }

    msg_extract_from_rx(COMM_IFACE_USART);

    if (comm_buffer_tx_count(COMM_IFACE_USART) == 0 && comm_interface_is_response_pending(COMM_IFACE_USART))
    {
        comm_interface_set_response_pending(COMM_IFACE_USART, false);
    }

    if (msg_is_ready(COMM_IFACE_USART) && !prev_msg_ready)
    {
        if (comm_buffer_tx_count(COMM_IFACE_USART) > 0 && msg_contains_query(COMM_IFACE_USART))
        {
            const uint8_t error_prefix[] = "-410;";
            comm_buffer_tx_prepend(COMM_IFACE_USART, error_prefix, 4);
            error_set(ERROR_QUERY_INTERRUPTED);
        }
        msg_ready_flag = true;
        prev_msg_ready = true;
    }

    if (!msg_is_ready(COMM_IFACE_USART))
    {
        prev_msg_ready = false;
    }

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

bool task_system_msg_available(void)
{
    return msg_ready_flag;
}

void task_system_msg_clear(void)
{
    msg_ready_flag = false;
}