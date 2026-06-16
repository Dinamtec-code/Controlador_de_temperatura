#include "tasks/task_system.h"
#include "tasks/task_comm.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include "communication/comm_message_buffer.h"
#include "services/scpi_parser.h"
#include "services/error_handler.h"
#include "main.h"

void task_system(void)
{
    if (task_system_msg_available())
    {
        const char *msg = msg_get_next(COMM_IFACE_USART);
        if (msg)
        {
            HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_SET);
            scpi_process_message(msg);
            HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin, GPIO_PIN_RESET);
            msg_mark_processed(COMM_IFACE_USART);
            comm_interface_set_response_pending(COMM_IFACE_USART, true);
            task_system_msg_clear();
        }
    }
}