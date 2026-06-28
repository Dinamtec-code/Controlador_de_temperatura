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
} cb_t;

void cb_init(cb_t *cb, uint8_t *buffer, size_t size);
cb_status_t cb_put(cb_t *cb, uint8_t data);
cb_status_t cb_get(cb_t *cb, uint8_t *data);
cb_status_t cb_status(cb_t *cb);
size_t cb_count(cb_t *cb);
cb_status_t cb_clear(cb_t *cb);

#endif