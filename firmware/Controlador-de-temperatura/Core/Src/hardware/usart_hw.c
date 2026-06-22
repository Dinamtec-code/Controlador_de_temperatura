#include "hardware/usart_hw.h"
#include "usart.h"
#include "dma.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include "main.h"
#include <string.h>

comm_iface_event_t event;

static comm_iface_t usart_iface;

static bool usart_hw_send(const uint8_t *data, size_t len)
{
    return comm_buffer_tx_put(COMM_IFACE_USART, data, len);
}

static bool usart_hw_is_connected(void *context)
{
    return true;
}

static inline void comm_buffer_protect_rx(void) { __HAL_UART_DISABLE_IT(&huart2, UART_IT_IDLE); }
static inline void comm_buffer_unprotect_rx(void) { __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE); }

static inline void comm_buffer_protect_tx(void) { __HAL_DMA_DISABLE_IT(&hdma_usart2_tx, DMA_IT_HT); }
static inline void comm_buffer_unprotect_tx(void) { __HAL_DMA_ENABLE_IT(&hdma_usart2_tx, DMA_IT_HT); }

void usart_hw_init(uint8_t *dma_rx_cb_buffer, size_t rx_len, uint8_t *dma_tx_buffer, size_t tx_len)
{
    usart_iface.context = NULL;
    usart_iface.name = "USART2";
    usart_iface.state = COMM_STATE_NONE;
    usart_iface.id = COMM_IFACE_USART;
    usart_iface.rx_buffer = dma_rx_cb_buffer;
    usart_iface.tx_buffer = dma_tx_buffer;
    usart_iface.rx_len = rx_len;
    usart_iface.tx_len = tx_len;
    usart_iface.start_rx = usart_hw_start_rx;
    usart_iface.stop_rx = usart_hw_stop_rx;
    usart_iface.start_tx = usart_hw_start_tx;

    comm_register_interface(&usart_iface);
}

/* USART RX start helper function for HW abstraction layer */
void usart_hw_start_rx(void *context)
{
    (void)context;
    HAL_UARTEx_ReceiveToIdle_DMA(&huart2, usart_iface.rx_buffer, usart_iface.rx_len);
}

void usart_hw_stop_rx(void *context)
{
    (void)context;
    HAL_UART_DMAStop(&huart2);
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t data_size)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = comm_get_interface(COMM_IFACE_USART);

        if (iface)
        {
            iface->event = IFACE_EVENT_RX_DATA_AVAILABLE;
            iface->post_event(&iface->event);
        }

        /**
        uint32_t bytes_received = data_size;
        for (size_t i = 0; i < bytes_received; i++)
        {
            uint8_t byte = dma_uart_rx_buffer[i];
            comm_buffer_rx_put(COMM_IFACE_USART, byte);
        }
        usart_hw_start_rx(NULL);
        */
    }
}
bool usart_hw_is_tx_ready(void *context)
{
    (void)context;
    return huart2.gState == HAL_UART_STATE_READY;
}

bool usart_hw_start_tx(void *context)
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
        comm_iface_t *iface = comm_get_interface(COMM_IFACE_USART);
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