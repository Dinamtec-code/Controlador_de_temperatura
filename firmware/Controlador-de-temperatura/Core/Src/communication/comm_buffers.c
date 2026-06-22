#include "communication/comm_buffers.h"
#include "communication/comm_interface.h"
#include "services/error_handler.h"
#include "hardware/usart_hw.h"
#include <string.h>

static circular_buffer_t rx_buffers[COMM_IFACE_MAX];
static uint8_t rx_buffer_mem[COMM_IFACE_MAX][COMM_BUFFER_RX_SIZE];
static circular_buffer_t tx_buffers[COMM_IFACE_MAX];
static uint8_t tx_buffer_mem[COMM_IFACE_MAX][COMM_BUFFER_TX_SIZE];
static comm_iface_t *registered_interfaces[COMM_IFACE_MAX];

void comm_buffers_init(void)
{
    for (int i = 0; i < COMM_IFACE_MAX; i++)
    {
        cb_init(&rx_buffers[i], rx_buffer_mem[i], COMM_BUFFER_RX_SIZE);
        cb_init(&tx_buffers[i], tx_buffer_mem[i], COMM_BUFFER_TX_SIZE);
        registered_interfaces[i] = NULL;
    }
}

bool comm_buffer_rx_put(comm_iface_id_t iface_id, uint8_t data)
{
    if (iface_id >= COMM_IFACE_MAX)
        return false;
    comm_iface_t *iface = comm_get_interface(iface_id);
    if (iface)
    {
        iface->rx_protect();
        cb_status_t status = cb_put(&rx_buffers[iface_id], data);
        iface->rx_unprotect();
        return status == BUF_OK;
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
    }
    return false;
}

bool comm_buffer_tx_put(comm_iface_id_t iface_id, const uint8_t *data, size_t len)
{
    if (iface_id >= COMM_IFACE_MAX || len == 0)
    {
        return false;
    }

    comm_iface_t *iface = comm_get_interface(iface_id);
    if (iface)
    {
        iface->tx_protect();

        for (size_t i = 0; i < len; i++)
        {
            if (cb_put(&tx_buffers[iface_id], data[i]) != BUF_OK)
            {
                iface->tx_unprotect();
                error_set(ERROR_TX_BUFFER_FULL);
                return false;
            }
        }

        iface->tx_unprotect();
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
        return false;
    }
    return true;
}

size_t comm_buffer_rx_count(comm_iface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return 0;
    return cb_count(&rx_buffers[iface_id]);
}

size_t comm_buffer_tx_count(comm_iface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return 0;
    return cb_count(&tx_buffers[iface_id]);
}

bool comm_buffer_rx_get(comm_iface_id_t iface_id, uint8_t *data, size_t *len)
{
    if (iface_id >= COMM_IFACE_MAX || !data || *len == 0)
    {
        return false;
    }
    comm_iface_t *iface = comm_get_interface(iface_id);
    if (iface)
    {
        iface->rx_protect();
        size_t available = cb_count(&rx_buffers[iface_id]);
        size_t to_read = (available < *len) ? available : *len;

        for (size_t i = 0; i < to_read; i++)
        {
            if (cb_get(&rx_buffers[iface_id], &data[i]) != BUF_OK)
            {
                iface->rx_unprotect();
                return false;
            }
        }
        *len = to_read;

        iface->rx_unprotect();
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
        return false;
    }
    return true;
}

void comm_register_interface(comm_iface_t *iface)
{
    if (!iface || iface->id >= COMM_IFACE_MAX)
        return;
    registered_interfaces[iface->id] = iface;
}

void comm_buffer_rx_clear(comm_iface_id_t iface_id)
{
    if (iface_id >= COMM_IFACE_MAX)
        return;
    comm_iface_t *iface = comm_get_interface(iface_id);
    if (!iface)
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
        return;
    }
    iface->rx_protect();
    cb_clear(&rx_buffers[iface_id]);
    iface->rx_unprotect();
}

bool comm_buffer_tx_prepend(comm_iface_id_t iface_id, const uint8_t *data, size_t len)
{
    if (iface_id >= COMM_IFACE_MAX || !data || len == 0)
        return false;

    comm_iface_t *iface = comm_get_interface(iface_id);
    if (!iface)
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
        return false;
    }

    iface->tx_protect();

    size_t available = COMM_BUFFER_TX_SIZE - cb_count(&tx_buffers[iface_id]);
    if (available < len)
    {
        iface->tx_unprotect();
        error_set(ERROR_TX_BUFFER_FULL);
        return false;
    }

    circular_buffer_t *cb = &tx_buffers[iface_id];

    cb->tail = (cb->tail - len + cb->size) % cb->size;
    for (size_t i = 0; i < len; i++)
    {
        cb->buffer[(cb->tail + i) % cb->size] = data[i];
    }
    cb->count += len;

    iface->tx_unprotect();
    return true;
}

void comm_unregister_interface(comm_iface_t *iface)
{
    if (!iface || iface->id >= COMM_IFACE_MAX)
        return;
    registered_interfaces[iface->id] = NULL;
}

comm_iface_t *comm_get_interface(comm_iface_id_t id)
{
    if (id >= COMM_IFACE_MAX)
        return NULL;
    return registered_interfaces[id];
}

bool comm_buffer_tx_get(comm_iface_id_t iface_id, uint8_t *data, size_t *len)
{
    if (iface_id >= COMM_IFACE_MAX || !data || *len == 0)
        return false;

    comm_iface_t *iface = comm_get_interface(iface_id);
    if (iface)
    {
        size_t available = cb_count(&tx_buffers[iface_id]);
        size_t to_read = (available < *len) ? available : *len;
        iface->tx_protect();
        for (size_t i = 0; i < to_read; i++)
        {
            if (cb_get(&tx_buffers[iface_id], &data[i]) != BUF_OK)
            {
                iface->tx_unprotect();
                return false;
            }
        }
        *len = to_read;

        iface->tx_unprotect();
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
        return false;
    }
    return true;
}

void comm_interface_start_rx(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface && iface->start_rx)
        iface->start_rx(iface->context);
}

void comm_interface_stop_rx(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface && iface->stop_rx)
        iface->stop_rx(iface->context);
}

bool comm_interface_is_tx_ready(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface && iface->is_tx_ready)
        return iface->is_tx_ready(iface->context);
    return false;
}

bool comm_iface_start_tx(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface && iface->start_tx)
        iface->start_tx(iface->context);
    return true;
    return false;
}

bool comm_interface_send(comm_iface_id_t id, const uint8_t *data, size_t len)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (!iface || !data || len == 0)
        return false;
    return iface->send(data, len);
}

bool comm_interface_is_rx_active(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        return (iface->state & COMM_STATE_RX_ACTIVE) != 0;
    }
    error_set(ERROR_INTERFACE_NOT_REGISTERED);
    return false;
}

bool comm_interface_is_tx_busy(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    return iface && (iface->state & COMM_STATE_TX_BUSY) != 0;
}

bool comm_interface_has_error(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    return iface && (iface->state & COMM_STATE_ERROR) != 0;
}

void comm_interface_set_response_pending(comm_iface_id_t id, bool pending)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        if (pending)
        {
            iface->state |= COMM_STATE_RESPONSE_PENDING;
        }
        else
        {
            iface->state &= ~COMM_STATE_RESPONSE_PENDING;
        }
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
    }
}

bool comm_interface_is_response_pending(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    return iface && (iface->state & COMM_STATE_RESPONSE_PENDING) != 0;
}

void comm_interface_set_rx_active(comm_iface_id_t id, bool active)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        if (active)
        {
            iface->state |= COMM_STATE_RX_ACTIVE;
        }
        else
        {
            iface->state &= ~COMM_STATE_RX_ACTIVE;
        }
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
    }
}

void comm_interface_set_tx_busy(comm_iface_id_t id, bool busy)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        if (busy)
        {
            iface->state |= COMM_STATE_TX_BUSY;
        }
        else
        {
            iface->state &= ~COMM_STATE_TX_BUSY;
        }
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
    }
}

void comm_interface_set_error(comm_iface_id_t id, bool error)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        if (error)
        {
            iface->state |= COMM_STATE_ERROR;
        }
        else
        {
            iface->state &= ~COMM_STATE_ERROR;
        }
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
    }
}

void comm_interface_set_state(comm_iface_id_t id, comm_iface_state_t state)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        iface->state = state;
    }
    else
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
    }
}

comm_iface_state_t comm_interface_get_state(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (iface)
    {
        return iface->state;
    }
    error_set(ERROR_INTERFACE_NOT_REGISTERED);
    return COMM_STATE_NONE;
}

void comm_interface_reset(comm_iface_id_t id)
{
    comm_iface_t *iface = comm_get_interface(id);
    if (!iface)
    {
        error_set(ERROR_INTERFACE_NOT_REGISTERED);
        return;
    }

    comm_interface_stop_rx(id);
    iface->state = COMM_STATE_NONE;
    cb_clear(&rx_buffers[id]);
    cb_clear(&tx_buffers[id]);
    comm_interface_start_rx(id);
}