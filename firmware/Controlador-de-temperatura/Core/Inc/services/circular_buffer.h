#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdint.h>
#include <stddef.h>

/* Cuando se opera sobre el bufer este puede devolver los estados siguientes
 *  BUF_OK: la operacion salió bien y el buffer esta integro.
 *  BUF_EMPTY: el buffer esta vacio, no se puede leer.
 *  BUF_FULL: el buffer esta lleno, no se puede escribir.
 *  BUF_ERROR: error general, por ejemplo puntero nulo o tamaño cero.
 **/
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
    size_t head;
    size_t tail;
    size_t count;
} circular_buffer_t;

void cb_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);
cb_status_t cb_put(circular_buffer_t *cb, uint8_t data);
cb_status_t cb_get(circular_buffer_t *cb, uint8_t *data);
cb_status_t cb_status(circular_buffer_t *cb);
size_t cb_count(circular_buffer_t *cb);
cb_status_t cb_clear(circular_buffer_t *cb);

#endif