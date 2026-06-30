#include "services/circular_buffer.h"

// Función auxiliar privada para evitar duplicar lógica
size_t cb_count(cb_t *cb)
{
    if (cb == NULL || cb->buffer == NULL || cb->size == 0)
    {
        return 0;
    }

    size_t head = cb->head;
    size_t tail = cb->tail;

    if (head >= tail)
    {
        return (head - tail);
    }
    else
    {
        return (cb->size - tail + head);
    }
}

static bool cb_is_valid(cb_t *cb)
{
    return (cb != NULL && cb->buffer != NULL && cb->size > 0);
}

void cb_init(cb_t *cb, uint8_t *buffer, size_t size)
{
    if (cb == NULL || buffer == NULL || size == 0)
    {
        return;
    }
    cb->buffer = buffer;
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
}

cb_status_t cb_status(cb_t *cb)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }

    size_t count = cb_count(cb);
    if (count == 0)
    {
        return BUF_EMPTY;
    }
    else if (count >= (cb->size - 1)) // -1 porque en buffer circular el estado "lleno" suele ser size-1
    {
        return BUF_FULL;
    }
    else
    {
        return BUF_OK;
    }
}

cb_status_t cb_put(cb_t *cb, uint8_t data)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }

    // Calculamos dónde quedaría el head si insertamos
    size_t next_head = (cb->head + 1) % cb->size;

    // Si el siguiente head es igual al tail, estamos llenos
    if (next_head == cb->tail)
    {
        return BUF_FULL;
    }

    cb->buffer[cb->head] = data;
    cb->head = next_head;

    return BUF_OK;
}

cb_status_t cb_get(cb_t *cb, uint8_t *data)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }

    if (cb->head == cb->tail)
    {
        return BUF_EMPTY;
    }

    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;

    return BUF_OK;
}

cb_status_t cb_clear(cb_t *cb)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }
    cb->head = 0;
    cb->tail = 0;
    return BUF_OK;
}