#include "hardware/usart_hw.h"
#include "hardware/circular_buffer.h"
#include "usart.h"
#include "dma.h"

circular_buffer_t rx_circular_buffer;
static uint8_t rx_buffer[USART_HW_RX_BUFFER_SIZE];

void usart_hw_init(void)
{
    cb_init(&rx_circular_buffer, rx_buffer, USART_HW_RX_BUFFER_SIZE);
}

void usart_hw_start_rx(void)
{
    ReceptionStart();
}

void usart_hw_idle_handler(void)
{
    uint32_t bytes_received = DMA_RX_BUFFER_SIZE - hdma_usart2_rx.Instance->CNDTR;
    if (bytes_received > 0 && bytes_received <= SDIN_BUFFER_SIZE) {
        for (size_t i = 0; i < bytes_received; i++) {
            uint8_t byte = dma_uart_rx_buffer[i];
            if (cb_put(&rx_circular_buffer, byte) == BUF_FULL) {
            }
        }
    }
    ReceptionStart();
}

void usart_hw_send_str(const char *str)
{
    for (size_t i = 0; str[i] != '\0'; i++) {
        sendChar(str[i]);
    }
    TransmitionStart(&TxbufferHandler);
}

void usart_hw_send_buf(uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        sendChar(data[i]);
    }
    TransmitionStart(&TxbufferHandler);
}