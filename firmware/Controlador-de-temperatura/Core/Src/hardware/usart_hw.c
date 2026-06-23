#include "hardware/usart_hw.h"
#include "usart.h"
#include "dma.h"
#include "communication/comm_interface.h"
#include "communication/comm_buffers.h"
#include "main.h"
#include <string.h>

extern DMA_HandleTypeDef hdma_usart2_tx;
extern UART_HandleTypeDef huart2;

static comm_iface_t usart_iface;

/* --- 1. CAPA DE PERIFÉRICO (Hardware puro) --- */

/**
 * @brief Configura físicamente el periférico USART2 en el silicio.
 */
static comm_error_t usart_hw_configure(void *ctx) {
    (void)ctx;
    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1;
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
    
    if (HAL_UART_Init(&huart2) != HAL_OK)
    {
        return COMM_ERR_INTERNAL; 
    }
   
    return COMM_ERR_NONE;
}

/**
 * @brief Desenergiza y apaga el periférico.
 */
static void usart_hw_deinit(void *ctx) {
    (void)ctx;
    HAL_UART_MspDeInit(&huart2);
}

/**
 * @brief Reinicia el hardware a su estado inicial.
 */
static comm_error_t usart_hw_reset(void *ctx) {
    return usart_hw_configure(ctx);
}

/* --- 2. CAPA DE INTERFAZ (Abstracción del flujo y control de buffers) --- */

static inline void comm_buffer_protect_rx(void) {
    NVIC_DisableIRQ(USART2_IRQn); 
    __DSB(); 
}

static inline void comm_buffer_unprotect_rx(void) {
    NVIC_EnableIRQ(USART2_IRQn); 
}

static inline void comm_buffer_protect_tx(void) { 
    __HAL_DMA_DISABLE_IT(&hdma_usart2_tx, DMA_IT_HT); 
}

static inline void comm_buffer_unprotect_tx(void) { 
    __HAL_DMA_ENABLE_IT(&hdma_usart2_tx, DMA_IT_HT); 
}

/**
 * @brief Inicializa el objeto abstracto de interfaz, inyecta dependencias de memoria
 * y lo registra en el despachador global.
 */
void usart_iface_register(circular_buffer_t *dma_rx_cb_buffer, uint8_t *dma_tx_buffer, size_t tx_len)
{
    usart_iface.context = NULL;
    usart_iface.name = "USART2";
    usart_iface.state = COMM_STATE_NONE;
    usart_iface.id = COMM_IFACE_USART;
    usart_iface.event = IFACE_EVENT_NONE;
    usart_iface.rx_buffer = dma_rx_cb_buffer;
    usart_iface.tx_buffer = dma_tx_buffer;
    usace_iface.tx_len = tx_len;
    
    // Vinculación semántica de punteros a la capa de hardware
    usart_iface.init = usart_hw_configure;
    usart_iface.deinit = usart_hw_deinit;
    usart_iface.reset = usart_hw_reset;
    
    usart_iface.start_rx = usart_hw_start_rx;
    usart_iface.stop_rx = usart_hw_stop_rx;
    usart_iface.start_tx = usart_hw_start_tx;
    usart_iface.set_event = usart_hw_set_event;
    usart_iface.get_event = usart_hw_get_event;
    usart_iface.protect_rx = comm_buffer_protect_rx;
    usart_iface.unprotect_rx = comm_buffer_unprotect_rx;
    usart_iface.protect_tx = comm_buffer_protect_tx;
    usart_iface.unprotect_tx = comm_buffer_unprotect_tx;

    comm_register_interface(&usart_iface);
}

void usart_hw_start_rx(void *context)
{
    (void)context;
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (!iface) return;

    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart2, usart_iface.rx_buffer->buffer, usart_iface.rx_buffer->size) == HAL_OK)
    {
        iface->state |= COMM_STATE_RX_ACTIVE;
    }
    else
    {
        iface->state |= COMM_STATE_ERROR;
    }
}

void usart_hw_stop_rx(void *context)
{
    (void)context;
    HAL_UART_DMAStop(&huart2);
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (iface) {
        iface->state &= ~COMM_STATE_RX_ACTIVE;
    }
}

bool usart_hw_start_tx(void *context)
{
    (void)context;
    comm_iface_t *iface = comm_get_interface(COMM_IFACE_USART);
    if (!iface || huart2.gState != HAL_UART_STATE_READY) return false;

    size_t actual_len = iface->tx_len; 

    if (!comm_buffer_tx_get(COMM_IFACE_USART, iface->tx_buffer, &actual_len) || actual_len == 0)
    {
        return false;
    }

    iface->state |= COMM_STATE_TX_ACTIVE;
    HAL_UART_Transmit_DMA(&huart2, iface->tx_buffer, (uint16_t)actual_len);
    
    return true;
}

void usart_hw_set_event(void *ctx, comm_iface_event_t event_flag) {
    (void)ctx;
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    
    if (iface) {
        usart_iface.protect_rx(); 
        iface->event |= event_flag;
        usart_iface.unprotect_rx();
    }
}

comm_iface_event_t usart_hw_get_event(void *ctx) {
    (void)ctx;
    comm_iface_event_t pending_events = IFACE_EVENT_NONE;
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    
    if (iface) {
        usart_iface.protect_rx();   
        pending_events = iface->event;
        iface->event = IFACE_EVENT_NONE; 
        usart_iface.unprotect_rx();
    }
    
    return pending_events;
}

/* --- 3. CALLBACKS DE INTERRUPCIÓN (Traducción HW -> Eventos de Interfaz) --- */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t data_size)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = comm_get_interface(COMM_IFACE_USART);
        
        if (iface && iface->rx_buffer)
        {
            circular_buffer_t *cb = iface->rx_buffer;
            size_t new_head = (size_t)data_size;
            
            if (new_head == cb->tail && cb->count > 0)
            {
                usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_OVERFLOW);
            }
            
            if (new_head >= cb->tail) {
                cb->count = new_head - cb->tail;
            } else {
                cb->count = (cb->size - cb->tail) + new_head;
            }
            
            cb->head = new_head;

            usart_hw_set_event(NULL, IFACE_EVENT_RX_DATA_AVAILABLE);
            iface->state |= COMM_STATE_RX_ACTIVE;
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = comm_get_interface(COMM_IFACE_USART);
        if (iface)
        {
            iface->state &= ~COMM_STATE_TX_ACTIVE;
            usart_hw_set_event(NULL, IFACE_EVENT_TX_COMPLETE);
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = comm_get_interface(COMM_IFACE_USART);
        if (iface)
        {
            iface->state |= COMM_STATE_ERROR;
            
            uint32_t err = HAL_UART_GetError(huart);
            if (err & HAL_UART_ERROR_ORE) usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_OVERFLOW);
            if (err & HAL_UART_ERROR_FE)  usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_FRAMING);
            if (err & HAL_UART_ERROR_PE)  usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_PARITY);
            if (err & HAL_UART_ERROR_DMA) usart_hw_set_event(NULL, IFACE_EVENT_TX_ERROR_BUS_FAULT);
        }
    }
}