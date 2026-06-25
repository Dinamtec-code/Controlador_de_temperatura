#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef enum
{
    BUF_OK = 0,
    BUF_EMPTY,
    BUF_FULL,
    BUF_ERROR
} cb_status_t;

typedef struct
{
    uint8_t *buffer;
    size_t size;
    volatile size_t head;
    volatile size_t tail;
} circular_buffer_t;

void cb_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);
cb_status_t cb_put(circular_buffer_t *cb, uint8_t data);
cb_status_t cb_get(circular_buffer_t *cb, uint8_t *data);
cb_status_t cb_status(circular_buffer_t *cb);
size_t cb_count(circular_buffer_t *cb);
cb_status_t cb_clear(circular_buffer_t *cb);

#endif