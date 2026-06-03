#include "tasks/task_comm.h"
#include "hardware/circular_buffer.h"
#include "hardware/usart_hw.h"
#include "services/scpi_parser.h"

static char cmd_buffer[128];
static size_t cmd_len = 0;

void task_comm(void)
{
    uint8_t byte;
    while (cb_get(&rx_circular_buffer, &byte) == BUF_OK) {
        if (byte == '\n' || byte == '\r' || cmd_len >= sizeof(cmd_buffer) - 1) {
            cmd_buffer[cmd_len] = '\0';
            if (cmd_len > 0) {
                scpi_process_line(cmd_buffer);
            }
            cmd_len = 0;
        } else {
            cmd_buffer[cmd_len++] = (char)byte;
        }
    }
}