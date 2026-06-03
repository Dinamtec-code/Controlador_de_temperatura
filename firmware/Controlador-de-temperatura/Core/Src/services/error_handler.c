#include "services/error_handler.h"

static uint32_t error_flags = 0;

void error_handler_init(void)
{
    error_flags = 0;
}

void error_set(error_code_t code)
{
    if (code < ERROR_MAX) {
        error_flags |= (1U << code);
    }
}

int error_check(error_code_t code)
{
    if (code < ERROR_MAX) {
        return (error_flags & (1U << code)) != 0;
    }
    return 0;
}

void error_clear(error_code_t code)
{
    if (code < ERROR_MAX) {
        error_flags &= ~(1U << code);
    }
}