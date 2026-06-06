#include "hardware/circular_buffer.h"
#include <stdio.h>
#include <stdbool.h>

static bool cb_is_valid(circular_buffer_t *cb)
{
    if (cb == NULL || cb->buffer == NULL || cb->size == 0 || cb->count > cb->size)
    {
        return false;
    }
    return true;
}

void cb_init(circular_buffer_t *cb, uint8_t *buffer, size_t size)
{
    if (cb == NULL || buffer == NULL || size == 0)
    {
        return;
    }
    cb->buffer = buffer;
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

cb_status_t cb_status(circular_buffer_t *cb)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }
    if (cb->count == 0)
    {
        return BUF_EMPTY;
    }
    else if (cb->count >= cb->size)
    {
        return BUF_FULL;
    }
    else
    {
        return BUF_OK;
    }
}

cb_status_t cb_put(circular_buffer_t *cb, uint8_t data)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }

    if (cb->count >= cb->size)
    {
        return BUF_FULL;
    }
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;
    cb->count++;

    return BUF_OK;
}

cb_status_t cb_get(circular_buffer_t *cb, uint8_t *data)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }

    if (cb->count == 0)
    {
        return BUF_EMPTY;
    }
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;
    cb->count--;

    return BUF_OK;
}

size_t cb_count(circular_buffer_t *cb)
{
    if (!cb_is_valid(cb))
    {
        return 0;
    }
    return cb->count;
}

cb_status_t cb_clear(circular_buffer_t *cb)
{
    if (!cb_is_valid(cb))
    {
        return BUF_ERROR;
    }
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    return BUF_OK;
}