#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdint.h>

typedef enum
{
    ERROR_NONE = 0,
    ERROR_REMOTE_RX_OVERFLOW = 1,
    ERROR_TX_BUFFER_FULL = 2,
    ERROR_INTERFACE_NOT_REGISTERED = 4,
    ERROR_QUERY_INTERRUPTED = 8,
    ERROR_MAX
} error_code_t;

void error_handler_init(void);
void error_set(error_code_t code);
int error_check(error_code_t code);
void error_clear(error_code_t code);

#endif