#ifndef APP_MSG_API_H
#define APP_MSG_API_H

#include <stdint.h>
#include <stddef.h>

/* Eventos lógicos hacia la aplicación */
typedef enum
{
    APP_MSG_EVENT_NONE = 0x00,
    APP_MSG_EVENT_MSG_READY = (1u << 0),
    APP_MSG_EVENT_TX_DONE = (1u << 1),
    APP_MSG_EVENT_RX_ERROR = (1u << 2),
    APP_MSG_EVENT_TX_ERROR = (1u << 3),
    APP_MSG_EVENT_INTERNAL_ERROR = (1u << 4)
} app_msg_event_t;

typedef enum
{
    APP_MSG_ERROR_NONE = 0x00,
    APP_MSG_ERROR_RX = 0x01,
    APP_MSG_ERROR_TX = 0x02,
    APP_MSG_ERROR_INTERNAL = 0x04
} app_msg_error_t;

/* Códigos de retorno de la aplicación */
typedef enum
{
    APP_MSG_OK = 0x00,
    APP_MSG_ERROR = 0x01,
    APP_MSG_BUSY = 0x02
} app_msg_response_t;

typedef struct app_msg_iface app_msg_iface_t;
/*
struct app_msg_iface
{
    void *context;
    app_msg_response_t (*on_message_ready)(void *ctx, const uint8_t *msg, size_t len);
    app_msg_response_t (*on_tx_done)(void *ctx);
    void (*on_error)(void *ctx, app_msg_error_t error);
};
*/

typedef struct app_msg_iface
{
    void *context;
    void (*on_rx_data)(void *ctx, const uint8_t *data, size_t len);
    app_msg_response_t (*on_tx_done)(void *ctx);
    void (*on_error)(void *ctx, app_msg_error_t error);
} app_msg_iface_t;

#endif /* APP_MSG_API_H */