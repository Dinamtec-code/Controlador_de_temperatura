#include "tasks/task_comm.h"
#include "usart.h"
#include "hardware/circular_buffer.h"
#include "hardware/usart_hw.h"
#include "hardware/oled_hw.h"
#include "services/scpi_parser.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

static char cmd_buffer[128];
static size_t cmd_len = 0;

void task_comm(void)
{
    uint8_t byte;

    while (cb_get(&rx_circular_buffer, &byte) == BUF_OK)
    {
        if (byte == '\n' || byte == '\r' || byte == '\0' || cmd_len >= sizeof(cmd_buffer) - 1 || cb_count(&rx_circular_buffer) == 0)
        {
            cmd_buffer[cmd_len] = '\0';
            if (cmd_len > 0)
            {
                scpi_process_line(cmd_buffer);
                HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            }
            memset(cmd_buffer, 0, sizeof(cmd_buffer));
            cmd_len = 0;
            cb_clear(&rx_circular_buffer);
        }
        else
        {
            cmd_buffer[cmd_len++] = (char)byte;
        }
    }
    comm_task();
}