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

/* Códigos de retorno de la aplicación */
typedef enum
{
    APP_MSG_OK = 0x00,
    APP_MSG_ERROR = 0x01,
    APP_MSG_BUSY = 0x02
} app_msg_response_t;

typedef struct app_msg_iface app_msg_iface_t;

struct app_msg_iface
{
    void *context;

    /**
     * @brief Entrega un mensaje completo. El puntero es válido solo hasta retornar.
     * @return APP_MSG_OK si se procesó, APP_MSG_BUSY si la aplicación no puede atenderlo ahora.
     */
    app_msg_response_t (*on_message_ready)(void *ctx, const uint8_t *msg, size_t len);

    /**
     * @brief Notifica que la transmisión de una respuesta ha finalizado.
     */
    app_msg_response_t (*on_tx_done)(void *ctx);

    /**
     * @brief Notifica un error originado en la capa de comunicación.
     */
    void (*on_error)(void *ctx, app_msg_event_t event);
};

#endif /* APP_MSG_API_H */