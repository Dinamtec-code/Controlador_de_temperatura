#ifndef COMM_SYS_API_H
#define COMM_SYS_API_H

#include "comm_driver_api.h"
#include <stdbool.h>

/**
 * @brief Estados globales del subsistema de comunicación (visibles al sistema).
 */
typedef enum
{
    COMM_SYS_STATE_DOWN = 0x00,     /**< Sin inicializar o detenido */
    COMM_SYS_STATE_READY = 0x01,    /**< Interfaz inicializada, sin RX activo */
    COMM_SYS_STATE_ACTIVE = 0x02,   /**< Recibiendo y transmitiendo */
    COMM_SYS_STATE_ERROR = 0x03,    /**< Falla persistente */
    COMM_SYS_STATE_SELECTING = 0x04 /**< Cambiando de medio de transporte */
} comm_sys_state_t;

typedef enum
{
    COMM_SYS_CMD_NONE = 0x00,
    COMM_SYS_CMD_START = (1u << 0),
    COMM_SYS_CMD_STOP = (1u << 1),
    COMM_SYS_CMD_RESET = (1u << 2)
} comm_sys_cmd_t;

/**
 * @brief Eventos que el subsistema notifica al sistema/usuario.
 */
typedef enum
{
    COMM_SYS_EVENT_STATE_CHANGED = (1u << 0), /**< Cambió el estado global */
    COMM_SYS_EVENT_IFACE_CHANGED = (1u << 1), /**< Cambió la interfaz activa */
    COMM_SYS_EVENT_ERROR = (1u << 2),         /**< Ocurrió un error crítico */
    COMM_SYS_EVENT_CONNECTED = (1u << 3),     /**< Se estableció conexión */
    COMM_SYS_EVENT_DISCONNECTED = (1u << 4)   /**< Se perdió conexión */
} comm_sys_event_t;

/**
 * @brief Callback para notificaciones asíncronas del subsistema.
 */
typedef void (*comm_sys_notify_cb_t)(comm_sys_event_t event, void *context);

/* ========================================================================== */
/* API de Control                                                             */
/* ========================================================================== */

/**
 * @brief Inicia el subsistema de comunicación.
 * @return COMM_IFACE_OK si se inició correctamente.
 */
comm_response_t comm_sys_start(void);

/**
 * @brief Detiene el subsistema de comunicación.
 * @return COMM_IFACE_OK si se detuvo correctamente.
 */
comm_response_t comm_sys_stop(void);

/**
 * @brief Reinicia la interfaz activa sin cambiar de medio.
 * @return COMM_IFACE_OK si el reinicio fue exitoso.
 */
comm_response_t comm_sys_reset_active_iface(void);

/* ========================================================================== */
/* API de Consulta                                                            */
/* ========================================================================== */

/**
 * @brief Obtiene el estado global del subsistema.
 */
comm_sys_state_t comm_sys_get_state(void);

/**
 * @brief Indica si la interfaz activa tiene conexión establecida.
 */
bool comm_sys_is_connected(void);

/**
 * @brief Obtiene información de la interfaz activa.
 * @param name  Salida: puntero al nombre (no modificar).
 * @param id    Salida: identificador de la interfaz.
 * @return      COMM_IFACE_OK si hay interfaz activa, COMM_IFACE_ERROR si no.
 */
comm_response_t comm_sys_get_active_iface_info(const char **name, comm_iface_id_t *id);

/* ========================================================================== */
/* API de Notificaciones                                                      */
/* ========================================================================== */

/**
 * @brief Registra un callback para recibir notificaciones del subsistema.
 * @param cb       Función a invocar ante eventos.
 * @param context  Puntero opaco pasado al callback.
 */
void comm_sys_register_notify_callback(comm_sys_notify_cb_t cb, void *context);

#endif /* COMM_SYS_API_H */