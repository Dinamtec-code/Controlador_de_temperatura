#include "hardware/usart_hw.h"
#include "usart.h"
#include "dma.h"
#include "communication/comm_driver_api.h"
#include "communication/comm_iface_registry.h"
#include "services/circular_buffer.h"
#include "main.h"

extern DMA_HandleTypeDef hdma_usart2_tx;
extern UART_HandleTypeDef huart2;
static comm_iface_t usart_iface;

inline static void full_protect(void *ctx)
{
    comm_iface_t *iface = (comm_iface_t *)ctx;
    iface->protect_rx(iface);
    iface->protect_tx(iface);
}

inline static void full_unprotect(void *ctx)
{
    comm_iface_t *iface = (comm_iface_t *)ctx;
    iface->unprotect_rx(iface);
    iface->unprotect_tx(iface);
}

static comm_error_t usart_hw_configure(void *ctx)
{

    comm_iface_t *iface = (comm_iface_t *)ctx;
    if (iface == NULL)
    {
        return COMM_ERR_NONE;
    }

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
        full_protect(ctx);
        iface->state = COMM_STATE_NONE;
        iface->event |= IFACE_EVENT_INTERFACE_DISCONNECTED;
        full_unprotect(ctx);

        return COMM_ERR_INTERNAL;
    }

    full_protect(ctx);
    iface->state |= COMM_STATE_CONNECTED;
    iface->state &= ~COMM_STATE_ERROR;
    iface->event |= IFACE_EVENT_INTERFACE_CONNECTED;
    full_unprotect(ctx);

    return COMM_ERR_NONE;
}

static comm_error_t usart_hw_deinit(void *ctx)
{
    if (huart2.Instance == NULL)
    {
        return COMM_ERR_INTERNAL;
    }
    HAL_UART_MspDeInit(&huart2);

    comm_iface_t *iface = (comm_iface_t *)ctx;
    if (iface != NULL)
    {
        full_protect(ctx);
        iface->state = COMM_STATE_NONE;
        iface->event |= IFACE_EVENT_INTERFACE_DISCONNECTED;
        full_unprotect(ctx);
    }

    return COMM_ERR_NONE;
}

static comm_error_t usart_hw_reset(void *ctx)
{
    return usart_hw_configure(ctx);
}

/* --- 2. CAPA DE INTERFAZ (Abstracción del flujo y control de buffers) --- */

static inline void comm_protect_rx(void *ctx)
{
    (void)ctx;
    NVIC_DisableIRQ(USART2_IRQn);
    __DSB();
}

static inline void comm_unprotect_rx(void *ctx)
{
    (void)ctx;
    NVIC_EnableIRQ(USART2_IRQn);
    __DSB();
}

static inline void comm_protect_tx(void *ctx)
{
    (void)ctx;
    __HAL_DMA_DISABLE_IT(&hdma_usart2_tx, DMA_IT_HT);
    __DSB();
}

static inline void comm_unprotect_tx(void *ctx)
{
    (void)ctx;
    __HAL_DMA_ENABLE_IT(&hdma_usart2_tx, DMA_IT_HT);
    __DSB();
}

comm_response_t usart_iface_register(cb_t *rx_cb, cb_t *tx_cb)
{
    if (rx_cb == NULL || tx_cb == NULL)
    {
        return COMM_IFACE_ERROR;
    }
    usart_iface.context = &usart_iface;
    usart_iface.name = "USART2";
    usart_iface.state = COMM_STATE_NONE;
    usart_iface.id = COMM_IFACE_USART;
    usart_iface.event = IFACE_EVENT_NONE;

    // Inyección de dependencias de la Tarea de Comunicación
    usart_iface.rx_buffer = rx_cb;
    usart_iface.tx_buffer = tx_cb;
    usart_iface.init = usart_hw_configure;
    usart_iface.deinit = usart_hw_deinit;
    usart_iface.reset = usart_hw_reset;

    usart_iface.start_rx = usart_hw_start_rx;
    usart_iface.stop_rx = usart_hw_stop_rx;
    usart_iface.start_tx = usart_hw_start_tx;

    usart_iface.get_event = usart_hw_get_event;
    usart_iface.protect_rx = comm_protect_rx;
    usart_iface.unprotect_rx = comm_unprotect_rx;
    usart_iface.protect_tx = comm_protect_tx;
    usart_iface.unprotect_tx = comm_unprotect_tx;

    comm_register_iface(&usart_iface);
    return COMM_IFACE_OK;
}

/* --- CONTROL DE FLUJO Y DMA --- */

comm_response_t usart_hw_start_rx(void *ctx)
{
    comm_iface_t *iface = (comm_iface_t *)ctx;
    if (!iface)
        return COMM_IFACE_ERROR;

    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart2, iface->rx_buffer->buffer, iface->rx_buffer->size) == HAL_OK)
    {
        full_protect(ctx);
        iface->state |= COMM_STATE_RX_ACTIVE;
        iface->state &= ~COMM_STATE_ERROR;
        full_unprotect(ctx);

        return COMM_IFACE_OK;
    }
    else
    {
        full_protect(ctx);
        iface->state &= ~COMM_STATE_RX_ACTIVE;
        iface->state |= COMM_STATE_ERROR;
        iface->event |= IFACE_EVENT_INTERNAL_ERROR;
        full_unprotect(ctx);

        return COMM_IFACE_ERROR;
    }
}

comm_response_t usart_hw_stop_rx(void *ctx)
{
    HAL_UART_DMAStop(&huart2);

    comm_iface_t *iface = (comm_iface_t *)ctx;
    if (!iface)
    {
        return COMM_IFACE_ERROR; // sin protecciones ni eventos
    }

    full_protect(ctx);
    iface->state &= ~COMM_STATE_RX_ACTIVE;
    full_unprotect(ctx);

    return COMM_IFACE_OK;
}

comm_response_t usart_hw_start_tx(void *ctx)
{
    comm_iface_t *iface = (comm_iface_t *)ctx;
    if (!iface || !iface->tx_buffer)
    {
        return COMM_IFACE_ERROR;
    }

    if (huart2.gState != HAL_UART_STATE_READY)
    {
        full_protect(ctx);
        iface->state |= COMM_STATE_ERROR;
        iface->event |= IFACE_EVENT_TX_ERROR_BUS_FAULT;
        full_unprotect(ctx);

        return COMM_IFACE_BUSY;
    }

    full_protect(ctx);
    size_t head = iface->tx_buffer->head;
    size_t tail = iface->tx_buffer->tail;
    size_t used = (head >= tail) ? (head - tail) : (iface->tx_buffer->size - tail + head);

    if (used == 0)
    {
        full_unprotect(ctx);
        return COMM_IFACE_IDLE; /* No es un error, simplemente no hay datos para enviar */
    }

    size_t contiguous_len = iface->tx_buffer->size - iface->tx_buffer->tail;
    size_t tx_len = (used < contiguous_len) ? used : contiguous_len;
    uint8_t *tx_ptr = &iface->tx_buffer->buffer[iface->tx_buffer->tail];

    /* 2. Intentamos disparar el hardware ANTES de tocar matemáticamente el buffer */
    if (HAL_UART_Transmit_DMA(&huart2, tx_ptr, (uint16_t)tx_len) != HAL_OK)
    {
        iface->state |= COMM_STATE_ERROR;
        iface->event |= IFACE_EVENT_TX_ERROR_BUS_FAULT;
        full_unprotect(ctx);
        return COMM_IFACE_ERROR;
    }

    /* 3. Si el hardware aceptó el envío, actualizamos el buffer y los estados */
    iface->tx_buffer->tail = (iface->tx_buffer->tail + tx_len) % iface->tx_buffer->size;
    iface->state |= COMM_STATE_TX_ACTIVE;
    iface->state &= ~COMM_STATE_ERROR; /* Limpiamos error previo si la transmisión fluyó */
    full_unprotect(ctx);

    return COMM_IFACE_OK;
}

/* --- GESTIÓN DE EVENTOS --- */

comm_iface_event_t usart_hw_get_event(void *ctx)
{
    comm_iface_event_t pending_events = IFACE_EVENT_NONE;
    comm_iface_t *iface = (comm_iface_t *)ctx;
    if (iface)
    {
        iface->protect_rx(ctx);
        pending_events = iface->event;
        iface->event = IFACE_EVENT_NONE;
        iface->unprotect_rx(ctx);
    }

    return pending_events;
}

/* --- 3. CALLBACKS DE INTERRUPCIÓN (Traducción HW -> Eventos de Interfaz) --- */

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t data_size)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = &usart_iface;
        if (iface && iface->rx_buffer)
        {
            full_protect(iface);

            size_t new_head = (size_t)data_size;
            size_t current_tail = iface->rx_buffer->tail;
            size_t next_pos = (new_head + 1) % iface->rx_buffer->size;

            if (next_pos == current_tail)
            {
                iface->event |= IFACE_EVENT_RX_ERROR_OVERFLOW;
                iface->state |= COMM_STATE_ERROR;
            }

            iface->rx_buffer->head = new_head;
            __DSB();
            iface->event |= IFACE_EVENT_RX_DATA_AVAILABLE;
            iface->state |= COMM_STATE_RX_ACTIVE;

            full_unprotect(iface);
        }
    }
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = &usart_iface;
        if (iface)
        {
            full_protect(iface); // TX, no RX
            iface->state &= ~COMM_STATE_TX_ACTIVE;
            iface->event |= IFACE_EVENT_TX_COMPLETE;
            full_unprotect(iface);
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        comm_iface_t *iface = &usart_iface;
        if (iface)
        {
            full_protect(iface);
            iface->state |= COMM_STATE_ERROR;
            uint32_t err = HAL_UART_GetError(huart);
            if (err & HAL_UART_ERROR_ORE)
                iface->event |= IFACE_EVENT_RX_ERROR_OVERFLOW;
            if (err & HAL_UART_ERROR_FE)
                iface->event |= IFACE_EVENT_RX_ERROR_FRAMING;
            if (err & HAL_UART_ERROR_PE)
                iface->event |= IFACE_EVENT_RX_ERROR_PARITY;
            if (err & HAL_UART_ERROR_DMA)
                iface->event |= IFACE_EVENT_TX_ERROR_BUS_FAULT;
            full_unprotect(iface);
        }
    }
}