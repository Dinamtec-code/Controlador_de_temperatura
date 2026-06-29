/**
 * @file    comm_sys_api.c
 * @brief   Implementación de la API de sistema del subsistema de comunicación.
 *
 * Este módulo permite que el resto del firmware (Motor SCPI, panel frontal,
 * pantalla) controle el subsistema de comunicación sin conocer sus detalles
 * internos.
 */

#include "comm_sys_api.h"
#include "comm_iface_registry.h"
#include <stddef.h>

/* ========================================================================== */
/* VARIABLES INTERNAS                                                        */
/* ========================================================================== */

/** Comandos pendientes publicados por las funciones de control. */
static comm_sys_cmd_t pending_cmds = COMM_SYS_CMD_NONE;

/** Solicitud de cambio de interfaz pendiente. */
static comm_iface_id_t pending_iface_request = COMM_IFACE_MAX;

/** Callback registrado para notificaciones asíncronas. */
static comm_sys_notify_cb_t notify_cb = NULL;
static void *notify_ctx = NULL;

/* ========================================================================== */
/* FUNCIONES INTERNAS (usadas por la Communication Task)                      */
/* ========================================================================== */

/**
 * @brief Publica un comando del sistema para ser consumido por la FSM de Gestión.
 *
 * Esta función es invocada por las funciones públicas de control
 * (comm_sys_start, comm_sys_stop, comm_sys_reset_active_iface).
 * No es parte de la API pública.
 */
void comm_sys_post_command(comm_sys_cmd_t cmd)
{
    pending_cmds |= cmd;
}

/**
 * @brief Consume atómicamente todos los comandos pendientes.
 *
 * Invocada por la FSM de Gestión al inicio de cada ciclo.
 * Retorna los comandos acumulados y limpia el registro.
 */
comm_sys_cmd_t comm_sys_consume_commands(void)
{
    comm_sys_cmd_t cmds = pending_cmds;
    pending_cmds = COMM_SYS_CMD_NONE;
    return cmds;
}

/**
 * @brief Solicita un cambio de interfaz activa.
 *
 * Invocada desde la aplicación cuando el usuario selecciona otro medio.
 * La FSM de Gestión evaluará la solicitud en su próximo ciclo.
 */
comm_response_t comm_sys_request_iface_change(comm_iface_id_t id)
{
    if (id >= COMM_IFACE_MAX)
    {
        return COMM_IFACE_ERROR;
    }
    pending_iface_request = id;
    return COMM_IFACE_OK;
}

/**
 * @brief Consume la solicitud de cambio de interfaz pendiente.
 *
 * Invocada por la FSM de Gestión para obtener y limpiar la solicitud.
 * Retorna COMM_IFACE_MAX si no hay solicitud pendiente.
 */
comm_iface_id_t comm_sys_consume_pending_request(void)
{
    comm_iface_id_t req = pending_iface_request;
    pending_iface_request = COMM_IFACE_MAX;
    return req;
}

/**
 * @brief Notifica un evento del subsistema al callback registrado.
 *
 * Invocada por la FSM de Gestión cuando ocurre un cambio de estado,
 * cambio de interfaz, error, etc.
 */
void comm_sys_notify_event(comm_sys_event_t event)
{
    if (notify_cb != NULL)
    {
        notify_cb(event, notify_ctx);
    }
}

/* ========================================================================== */
/* API PÚBLICA DE CONTROL                                                    */
/* ========================================================================== */

comm_response_t comm_sys_start(void)
{
    comm_sys_post_command(COMM_SYS_CMD_START);
    return COMM_IFACE_OK;
}

comm_response_t comm_sys_stop(void)
{
    comm_sys_post_command(COMM_SYS_CMD_STOP);
    return COMM_IFACE_OK;
}

comm_response_t comm_sys_reset_active_iface(void)
{
    comm_sys_post_command(COMM_SYS_CMD_RESET);
    return COMM_IFACE_OK;
}

/* ========================================================================== */
/* API PÚBLICA DE CONSULTA                                                    */
/* ========================================================================== */

comm_sys_state_t comm_sys_get_state(void)
{
    extern comm_sys_state_t comm_task_get_sys_state(void);
    return comm_task_get_sys_state();
}

bool comm_sys_is_connected(void)
{
    comm_iface_t *iface = comm_sys_get_active_iface();
    if (iface != NULL && (iface->state & COMM_STATE_CONNECTED))
    {
        return true;
    }
    return false;
}

comm_iface_t *comm_sys_get_active_iface(void)
{
    extern comm_iface_t *comm_task_get_active_iface(void);
    return comm_task_get_active_iface();
}

comm_response_t comm_sys_get_active_iface_info(const char **name, comm_iface_id_t *id)
{
    comm_iface_t *iface = comm_sys_get_active_iface();
    if (iface == NULL)
    {
        return COMM_IFACE_ERROR;
    }
    if (name != NULL)
    {
        *name = iface->name;
    }
    if (id != NULL)
    {
        *id = iface->id;
    }
    return COMM_IFACE_OK;
}

/* ========================================================================== */
/* API PÚBLICA DE NOTIFICACIONES                                              */
/* ========================================================================== */

void comm_sys_register_notify_callback(comm_sys_notify_cb_t cb, void *context)
{
    notify_cb = cb;
    notify_ctx = context;
}