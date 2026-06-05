#include "hardware/usart_hw.h"
#include "usart.h"
#include "dma.h"
#include <string.h>

/* Buffer de recepción circular */
circular_buffer_t rx_circular_buffer;
static uint8_t rx_buffer[USART_HW_RX_BUFFER_SIZE];

/* Buffer de transmisión DMA */
static uint8_t tx_buffer[USART_HW_TX_BUFFER_SIZE];
static volatile size_t tx_head = 0;
static volatile size_t tx_tail = 0;
static volatile size_t tx_count = 0;
static volatile bool tx_busy = false;

void usart_hw_init(void)
{
    cb_init(&rx_circular_buffer, rx_buffer, USART_HW_RX_BUFFER_SIZE);
}

void usart_hw_start_rx(void)
{
    HAL_UART_DMAStop(&huart2);
    uint32_t bytes_received = DMA_RX_BUFFER_SIZE - hdma_usart2_rx.Instance->CNDTR;
    for (size_t i = 0; i < bytes_received; i++)
    {
        uint8_t byte = dma_uart_rx_buffer[i];
        cb_put(&rx_circular_buffer, byte);
    }
    receptionStart();
}

void usart_hw_idle_handler(void)
{
    HAL_UART_DMAStop(&huart2);
    uint32_t bytes_received = DMA_RX_BUFFER_SIZE - hdma_usart2_rx.Instance->CNDTR;
    for (size_t i = 0; i < bytes_received; i++)
    {
        uint8_t byte = dma_uart_rx_buffer[i];
        cb_put(&rx_circular_buffer, byte);
    }
    receptionStart();
}

void usart_hw_send_str(const char *str)
{
    while (*str)
    {
        usart_hw_send_char((uint8_t)*str++);
    }
}

void usart_hw_send_buf(uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        usart_hw_send_char(data[i]);
    }
}

/* Función interna para encolar caracteres */
static void usart_hw_send_char(uint8_t ch)
{
    if (tx_count >= USART_HW_TX_BUFFER_SIZE)
    {
        return;
    }
    
    __disable_irq();
    tx_buffer[tx_head] = ch;
    tx_head = (tx_head + 1) % USART_HW_TX_BUFFER_SIZE;
    tx_count++;
    
    if (!tx_busy && huart2.gState == HAL_UART_STATE_READY)
    {
        usart_hw_start_tx();
    }
    __enable_irq();
}

/* Internal: start next DMA transmission if data pending */
static void usart_hw_start_tx(void)
{
    if (tx_count > 0 && huart2.gState == HAL_UART_STATE_READY)
    {
        tx_busy = true;
        size_t to_send = tx_count;
        if (tx_tail + to_send > USART_HW_TX_BUFFER_SIZE)
        {
            to_send = USART_HW_TX_BUFFER_SIZE - tx_tail;
        }
        
        HAL_UART_Transmit_DMA(&huart2, &tx_buffer[tx_tail], to_send);
        tx_tail = (tx_tail + to_send) % USART_HW_TX_BUFFER_SIZE;
        tx_count -= to_send;
    }
}

/* Called from HAL UART TX complete callback */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (tx_count > 0)
        {
            size_t to_send = tx_count;
            if (tx_tail + to_send > USART_HW_TX_BUFFER_SIZE)
            {
                to_send = USART_HW_TX_BUFFER_SIZE - tx_tail;
            }
            
            HAL_UART_Transmit_DMA(&huart2, &tx_buffer[tx_tail], to_send);
            tx_tail = (tx_tail + to_send) % USART_HW_TX_BUFFER_SIZE;
            tx_count -= to_send;
        }
        else
        {
            tx_busy = false;
        }
    }
}

bool usart_hw_is_tx_ready(void)
{
    return (huart2.gState == HAL_UART_STATE_READY);
}