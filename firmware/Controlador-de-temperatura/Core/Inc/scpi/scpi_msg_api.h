#ifndef APP_MSG_API_H
#define APP_MSG_API_H

#include <stdint.h>
#include <stddef.h>

/* --- Eventos lógicos hacia la aplicación --- */
typedef enum
{
    APP_MSG_EVENT_NONE = 0x00,
    APP_MSG_EVENT_MSG_READY = (1u << 0),     /**< Mensaje completo disponible */
    APP_MSG_EVENT_TX_DONE = (1u << 1),       /**< Transmisión completada */
    APP_MSG_EVENT_RX_ERROR = (1u << 2),      /**< Error durante la recepción */
    APP_MSG_EVENT_TX_ERROR = (1u << 3),      /**< Error durante la transmisión */
    APP_MSG_EVENT_INTERNAL_ERROR = (1u << 4) /**< Error interno del middleware */
} app_msg_event_t;

/* --- Códigos de retorno de la aplicación --- */
typedef enum
{
    APP_MSG_OK = 0x00,
    APP_MSG_ERROR = 0x01,
    APP_MSG_BUSY = 0x02
} app_msg_response_t;

/* Forward declaration */
typedef struct app_msg_iface app_msg_iface_t;

/**
 * @brief Interfaz que debe implementar la capa de aplicación.
 *
 * La Communication Task invoca estos callbacks para entregar mensajes
 * y notificar eventos. La aplicación es dueña de la memoria de los
 * mensajes que recibe.
 */
struct app_msg_iface
{
    void *context;

    app_msg_response_t (*on_message_ready)(void *ctx, const uint8_t *msg, size_t len);
    app_msg_response_t (*on_tx_done)(void *ctx);
    void (*on_error)(void *ctx, app_msg_event_t event);
};

#endif /* APP_MSG_API_H */