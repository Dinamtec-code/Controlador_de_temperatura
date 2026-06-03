#include "hardware/circular_buffer.h"

void cb_init(circular_buffer_t *cb, uint8_t *buffer, size_t size)
{
    cb->buffer = buffer;
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
}

cb_status_t cb_put(circular_buffer_t *cb, uint8_t data)
{
    if (cb->count >= cb->size) {
        return BUF_FULL;
    }
    cb->buffer[cb->head] = data;
    cb->head = (cb->head + 1) % cb->size;
    cb->count++;
    return BUF_OK;
}

cb_status_t cb_get(circular_buffer_t *cb, uint8_t *data)
{
    if (cb->count == 0) {
        return BUF_EMPTY;
    }
    *data = cb->buffer[cb->tail];
    cb->tail = (cb->tail + 1) % cb->size;
    cb->count--;
    return BUF_OK;
}

size_t cb_count(circular_buffer_t *cb)
{
    return cb->count;
}

cb_status_t cb_clear(circular_buffer_t *cb)
{
    cb->head = 0;
    cb->tail = 0;
    cb->count = 0;
    return BUF_OK;
}