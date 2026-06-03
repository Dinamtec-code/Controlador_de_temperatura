#include "tasks/task_system.h"
#include "hardware/usart_hw.h"
#include "services/error_handler.h"
#include "services/scpi_parser.h"

void task_system(void)
{
    if (error_check(ERROR_REMOTE_RX_OVERFLOW)) {
        usart_hw_send_str("ERR:RX_BUFFER_OVERFLOW\r\n");
        error_clear(ERROR_REMOTE_RX_OVERFLOW);
    }
}