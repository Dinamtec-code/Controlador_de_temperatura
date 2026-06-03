#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <stdint.h>

typedef enum {
    ERROR_NONE = 0,
    ERROR_REMOTE_RX_OVERFLOW = 1,
    ERROR_MAX
} error_code_t;

void error_handler_init(void);
void error_set(error_code_t code);
int error_check(error_code_t code);
void error_clear(error_code_t code);

#endif