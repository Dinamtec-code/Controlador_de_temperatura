#include "hardware/usart_hw.h"
#include "usart.h"
#include "dma.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include <string.h>

static comm_interface_t usart_interface;

static bool usart_hw_send(const uint8_t *data, size_t len)
{
    return comm_buffer_tx_put(COMM_IFACE_USART, data, len);
}

static bool usart_hw_is_connected(void *context)
{
    return true;
}

void usart_hw_init(void)
{
    usart_interface.id = COMM_IFACE_USART;
    usart_interface.context = NULL;
    usart_interface.name = "USART2";
    usart_interface.send = usart_hw_send;
    usart_interface.is_connected = usart_hw_is_connected;
    usart_interface.start_rx = usart_hw_start_rx;
    usart_interface.stop_rx = usart_hw_stop_rx;
    usart_interface.is_tx_ready = usart_hw_is_tx_ready;
    usart_interface.start_tx = usart_hw_transmit_from_system_buffer;
    usart_interface.rx_indication_cb = NULL;
    usart_interface.tx_complete_cb = NULL;

    comm_register_interface(&usart_interface);
    usart_interface.state = COMM_STATE_NONE;
}

/* USART RX start helper function for HW abstraction layer */
void usart_hw_start_rx(void *context)
{
    (void)context;
    memset(dma_uart_rx_buffer, 0, DMA_RX_BUFFER_SIZE);
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);
    HAL_UART_Receive_DMA(&huart2, dma_uart_rx_buffer, DMA_RX_BUFFER_SIZE);
    __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);

}

void usart_hw_stop_rx(void *context)
{
    (void)context;
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_IDLE);
    HAL_UART_DMAStop(&huart2);
}

void usart_hw_idle_handler(void)
{
    HAL_UART_DMAStop(&huart2);
    uint32_t bytes_received = DMA_RX_BUFFER_SIZE - hdma_usart2_rx.Instance->CNDTR;

    for (size_t i = 0; i < bytes_received; i++)
    {
        uint8_t byte = dma_uart_rx_buffer[i];
        comm_buffer_rx_put(COMM_IFACE_USART, byte);
    }

    usart_hw_start_rx(NULL);
}

void usart_hw_send_str(const char *str)
{
    while (*str)
    {
        usart_interface.send((const uint8_t *)str, 1);
        str++;
    }
}

void usart_hw_send_buf(uint8_t *data, size_t len)
{
    usart_interface.send(data, len);
}

bool usart_hw_is_tx_ready(void *context)
{
    (void)context;
    return huart2.gState == HAL_UART_STATE_READY;
}

bool usart_hw_transmit_from_system_buffer(void *context)
{
    (void)context;
    if (huart2.gState != HAL_UART_STATE_READY)
    {
        return false;
    }

    uint8_t chunk[COMM_BUFFER_TX_SIZE];
    size_t chunk_len = COMM_BUFFER_TX_SIZE;

    if (!comm_buffer_tx_get(COMM_IFACE_USART, chunk, &chunk_len) || chunk_len == 0)
    {
        return false;
    }

    HAL_UART_Transmit_DMA(&huart2, chunk, chunk_len);
    return true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        comm_interface_t *iface = comm_get_interface(COMM_IFACE_USART);
        comm_interface_set_tx_busy(COMM_IFACE_USART, false);

        if (iface && iface->tx_complete_cb)
        {
            iface->tx_complete_cb(COMM_IFACE_USART);
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        comm_interface_set_error(COMM_IFACE_USART, true);
    }
}