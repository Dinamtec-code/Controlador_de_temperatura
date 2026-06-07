#include "tasks/task_comm.h"
#include "hardware/usart_hw.h"
#include "services/scpi_parser.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include "main.h"
#include <stdio.h>
#include <string.h>

static char cmd_buffer[128];
static size_t cmd_len = 0;

static void send_response_to_interface(const char *resp, void *context) {
    comm_buffer_tx_put(COMM_IFACE_USART, (const uint8_t *)resp, strlen(resp));
}

void task_comm(void) {
    static bool output_iface_set = false;
    
    if (!output_iface_set) {
        scpi_output_interface_t out_iface = {
            .send_response = send_response_to_interface,
            .context = NULL
        };
        scpi_set_output_interface(&out_iface);
        output_iface_set = true;
    }
    
    uint8_t byte;
    size_t available = comm_buffer_rx_count(COMM_IFACE_USART);
    
    for (size_t i = 0; i < available && cmd_len < sizeof(cmd_buffer) - 1; i++) {
        size_t read_len = 1;
        if (comm_buffer_rx_get(COMM_IFACE_USART, &byte, &read_len) != true) {
            continue;
        }
        
        if (byte == '\n' || byte == '\r' || byte == '\0') {
            cmd_buffer[cmd_len] = '\0';
            if (cmd_len > 0) {
                scpi_process_line(cmd_buffer);
                HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
            }
            cmd_len = 0;
        } else {
            cmd_buffer[cmd_len++] = (char)byte;
        }
    }
    
    if (usart_hw_is_tx_ready()) {
        usart_hw_transmit_from_system_buffer();
    }
}