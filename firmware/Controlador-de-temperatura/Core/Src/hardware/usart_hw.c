#include "hardware/usart_hw.h"
#include "usart.h"
#include "dma.h"
#include "communication/comm_interface.h"
#include "services/circular_buffer.h"
#include "main.h"

extern DMA_HandleTypeDef hdma_usart2_tx;
extern UART_HandleTypeDef huart2;
static comm_iface_t usart_iface;

static comm_error_t usart_hw_configure(void *ctx)
{
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

static void usart_hw_deinit(void *ctx)
{
    (void)ctx;
    HAL_UART_MspDeInit(&huart2);
}

static comm_error_t usart_hw_reset(void *ctx)
{
    return usart_hw_configure(ctx);
}

/* --- 2. CAPA DE INTERFAZ (Abstracción del flujo y control de buffers) --- */

static inline void comm_buffer_protect_rx(void)
{
    NVIC_DisableIRQ(USART2_IRQn);
    __DSB();
}

static inline void comm_buffer_unprotect_rx(void)
{
    NVIC_EnableIRQ(USART2_IRQn);
}

static inline void comm_buffer_protect_tx(void)
{
    __HAL_DMA_DISABLE_IT(&hdma_usart2_tx, DMA_IT_HT);
}

static inline void comm_buffer_unprotect_tx(void)
{
    __HAL_DMA_ENABLE_IT(&hdma_usart2_tx, DMA_IT_HT);
}

void usart_iface_register(circular_buffer_t *rx_cb, circular_buffer_t *tx_cb)
{
    usart_iface.context = NULL;
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

    usart_iface.get_char_rx = usart_hw_get_char_rx;
    usart_iface.put_char_tx = usart_hw_put_char_tx;

    usart_iface.set_event = usart_hw_set_event;
    usart_iface.get_event = usart_hw_get_event;
    usart_iface.protect_rx = comm_buffer_protect_rx;
    usart_iface.unprotect_rx = comm_buffer_unprotect_rx;
    usart_iface.protect_tx = comm_buffer_protect_tx;
    usart_iface.unprotect_tx = comm_buffer_unprotect_tx;

    comm_register_interface(&usart_iface);
}

/* --- API DE ACCESO A DATOS --- */

bool usart_hw_get_char_rx(void *ctx, uint8_t *data)
{
    (void)ctx;

    comm_iface_t *iface = comm_get_interface(usart_iface.id);

    if (!iface || !iface->rx_buffer || !data)
    {
        return false;
    }

    iface->protect_rx();

    bool status = (cb_get(iface->rx_buffer, data) == BUF_OK);

    if (status && iface->rx_buffer->head == iface->rx_buffer->tail)
    {
        iface->state &= ~COMM_STATE_RX_ACTIVE;
    }
    iface->unprotect_rx();

    return status;
}

bool usart_hw_put_char_tx(void *ctx, uint8_t data)
{
    (void)ctx;

    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (!iface || !iface->tx_buffer)
    {
        return false;
    }
    iface->protect_tx();
    bool status = (cb_put(iface->tx_buffer, data) == BUF_OK);
    iface->unprotect_tx();
    return status;
}

/* --- CONTROL DE FLUJO Y DMA --- */

void usart_hw_start_rx(void *context)
{
    (void)context;

    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (!iface)
    {
        return;
    }
    if (HAL_UARTEx_ReceiveToIdle_DMA(&huart2, usart_iface.rx_buffer->buffer, usart_iface.rx_buffer->size) == HAL_OK)
    {
        iface->state |= COMM_STATE_RX_ACTIVE;
        iface->state &= ~COMM_STATE_ERROR; /* Limpiamos error previo si arrancó bien */
    }
    else
    {
        /* Sincronización estricta: El HW falló, la interfaz debe reportarlo */
        iface->state &= ~COMM_STATE_RX_ACTIVE;
        iface->state |= COMM_STATE_ERROR;
        usart_hw_set_event(NULL, IFACE_EVENT_INTERNAL_ERROR);
    }
}

void usart_hw_stop_rx(void *context)
{
    (void)context;
    HAL_UART_DMAStop(&huart2);
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (iface)
    {
        iface->state &= ~COMM_STATE_RX_ACTIVE;
    }
}

bool usart_hw_start_tx(void *context)
{
    (void)context;
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (!iface || !iface->tx_buffer)
    {
        return false;
    }

    /* 1. Sincronización de Estado: Si el HAL está trabado, la interfaz debe saberlo y reportarlo */

    if (huart2.gState != HAL_UART_STATE_READY)
    {
        iface->state |= COMM_STATE_ERROR;
        usart_hw_set_event(NULL, IFACE_EVENT_TX_ERROR_BUS_FAULT);
        return false;
    }
    iface->protect_tx();

    size_t head = iface->tx_buffer->head;
    size_t tail = iface->tx_buffer->tail;
    size_t used = (head >= tail) ? (head - tail) : (iface->tx_buffer->size - tail + head);

    if (used == 0)
    {
        iface->unprotect_tx();
        return false; /* No es un error, simplemente no hay datos para enviar */
    }

    size_t contiguous_len = iface->tx_buffer->size - iface->tx_buffer->tail;
    size_t tx_len = (used < contiguous_len) ? used : contiguous_len;
    uint8_t *tx_ptr = &iface->tx_buffer->buffer[iface->tx_buffer->tail];

    /* 2. Intentamos disparar el hardware ANTES de tocar matemáticamente el buffer */

    if (HAL_UART_Transmit_DMA(&huart2, tx_ptr, (uint16_t)tx_len) != HAL_OK)
    {
        iface->state |= COMM_STATE_ERROR;
        usart_hw_set_event(NULL, IFACE_EVENT_TX_ERROR_BUS_FAULT);
        iface->unprotect_tx();
        return false;
    }

    /* 3. Si el hardware aceptó el envío, actualizamos el buffer y los estados */

    iface->tx_buffer->tail = (iface->tx_buffer->tail + tx_len) % iface->tx_buffer->size;
    iface->state |= COMM_STATE_TX_ACTIVE;
    iface->state &= ~COMM_STATE_ERROR; /* Limpiamos error previo si la transmisión fluyó */
    iface->unprotect_tx();

    return true;
}

/* --- GESTIÓN DE EVENTOS --- */

void usart_hw_set_event(void *ctx, comm_iface_event_t event_flag)
{

    (void)ctx;
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (iface)
    {

        usart_iface.protect_rx();

        iface->event |= event_flag;

        usart_iface.unprotect_rx();
    }
}

comm_iface_event_t usart_hw_get_event(void *ctx)
{
    (void)ctx;

    comm_iface_event_t pending_events = IFACE_EVENT_NONE;
    comm_iface_t *iface = comm_get_interface(usart_iface.id);
    if (iface)
    {
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

            size_t new_head = (size_t)data_size;
            size_t current_tail = iface->rx_buffer->tail;
            // Si el nuevo head "alcanza" al tail, significa que el DMA

            // ha dado la vuelta completa y está pisando datos no leídos.
            size_t next_pos = (new_head + 1) % iface->rx_buffer->size;
            if (next_pos == current_tail)
            {
                // El DMA ya sobrescribió o está por sobrescribir datos críticos.
                usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_OVERFLOW);

                iface->state |= COMM_STATE_ERROR;
            }

            // ---------------------------------------
            iface->rx_buffer->head = new_head;
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
            if (err & HAL_UART_ERROR_ORE)
                usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_OVERFLOW);
            if (err & HAL_UART_ERROR_FE)
                usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_FRAMING);
            if (err & HAL_UART_ERROR_PE)
                usart_hw_set_event(NULL, IFACE_EVENT_RX_ERROR_PARITY);
            if (err & HAL_UART_ERROR_DMA)
                usart_hw_set_event(NULL, IFACE_EVENT_TX_ERROR_BUS_FAULT);
        }
    }
}
