#include "tasks/task_comm.h"
#include "config_manager.h"

#include "communication/comm_driver_api.h"
#include "communication/comm_sys_api.h"
#include "communication/app_msg_api.h"
#include "comm_iface_registry.h"

#include "services/error_handler.h"

#include "main.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Metodos privados commandos
 *
 ******************************************************************************/
extern void comm_sys_post_command(comm_sys_cmd_t cmd);
extern comm_sys_cmd_t comm_sys_consume_commands(void);

/*******************************************************************************
 * Registros del sistema
 *
 ******************************************************************************/
static comm_sys_state_t sys_state = COMM_SYS_STATE_DOWN;
static comm_sys_cmd_t pending_cmds = COMM_SYS_CMD_NONE;

/*******************************************************************************
 * Registros de la interfaz
 *
 ******************************************************************************/

static comm_iface_t *active_iface = NULL;
static comm_iface_event_t event_iface_snapshot = IFACE_EVENT_NONE;

/*******************************************************************************
 * Creación e inicialización de buffers de entrada y salida
 *
 ******************************************************************************/

static volatile uint8_t rx_buffer_mem[RX_BUFFER_SIZE];
static volatile cb_t rx_buffer = {
    .buffer = rx_buffer_mem,
    .size = RX_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
};

static volatile uint8_t tx_buffer_mem[RX_BUFFER_SIZE];
static volatile cb_t tx_buffer = {
    .buffer = tx_buffer_mem,
    .size = TX_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
};

static volatile bool msg_ready_flag = false;

comm_iface_t *comm_task_get_active_iface(void)
{
    return active_iface;
}

void comm_task_get_buffers(cb_t **rx_ptr, cb_t **tx_ptr)
{
    *rx_ptr = &rx_buffer;
    *tx_ptr = &tx_buffer;
}

static void send_response_to_interface(const char *resp, void *context)
{
    comm_buffer_tx_put(COMM_IFACE_USART, (const uint8_t *)resp, strlen(resp));
}

comm_sys_state_t comm_task_get_sys_state(void)
{
    return sys_state;
}

// -----------------------------------------------------------------------------
// FSM DE GESTIÓN DE INTERFAZ
// Estados: DOWN, READY, ACTIVE, ERROR, SELECTING
// -----------------------------------------------------------------------------

// Funciones auxiliares (selectora interna, notificación, cambio de estado)
static comm_iface_t *selector_get_active_iface(void)
{
    // 1. ¿Hay una solicitud de cambio pendiente desde el sistema?
    comm_iface_id_t req = comm_sys_consume_pending_request();
    if (req < COMM_IFACE_MAX)
    {
        comm_iface_t *iface = comm_get_iface(req);
        if (iface)
            return iface; // retorna la solicitada (aunque no esté conectada aún)
    }

    // 2. Leer configuración preferida (desde config_manager, cuando exista)
    comm_iface_id_t pref = config_get_preferred_iface();
    comm_iface_t *iface = comm_get_iface(pref);
    if (iface)
        return iface;

    // 3. Fallback: primera interfaz registrada
    for (int i = 0; i < COMM_IFACE_MAX; i++)
    {
        iface = comm_get_iface((comm_iface_id_t)i);
        if (iface)
            return iface;
    }
    return NULL;
}

static void sys_state_change(comm_sys_state_t new_state)
{
    if (sys_state != new_state)
    {
        sys_state = new_state;
        comm_sys_notify_event(COMM_SYS_EVENT_STATE_CHANGED);
    }
}

static void notify_iface_changed(void)
{
    comm_sys_notify_event(COMM_SYS_EVENT_IFACE_CHANGED);
}

void fsm_gestion(void)
{
    // --- Capturas atómicas al inicio del ciclo ---
    comm_sys_cmd_t sys_cmds = comm_sys_consume_commands();

    if (active_iface)
    {
        event_iface_snapshot = active_iface->get_event(active_iface->context);
    }

    // --- Evaluación por estado usando sys_cmds y event_iface_snapshot ---
    switch (sys_state)
    {
    case COMM_SYS_STATE_DOWN:
        if (sys_cmds & COMM_SYS_CMD_START)
        {
            comm_iface_t *iface = selector_get_active_iface();
            if (iface)
            {
                iface->init(iface->context);
                active_iface = iface;
                sys_state_change(COMM_SYS_STATE_READY);
                notify_iface_changed();
            }
        }
        break;

    case COMM_SYS_STATE_READY:
        if (event_iface_snapshot & IFACE_EVENT_INTERFACE_CONNECTED)
        {
            active_iface->start_rx(active_iface->context);
            sys_state_change(COMM_SYS_STATE_ACTIVE);
            comm_sys_notify_event(COMM_SYS_EVENT_CONNECTED);
        }
        if (sys_cmds & COMM_SYS_CMD_STOP)
        {
            active_iface->deinit(active_iface->context);
            active_iface = NULL;
            sys_state_change(COMM_SYS_STATE_DOWN);
        }
        break;

    case COMM_SYS_STATE_ACTIVE:
        if (event_iface_snapshot & (IFACE_EVENT_TX_ERROR_BUS_FAULT |
                                    IFACE_EVENT_INTERNAL_ERROR))
        {
            active_iface->stop_rx(active_iface->context);
            sys_state_change(COMM_SYS_STATE_ERROR);
            comm_sys_notify_event(COMM_SYS_EVENT_ERROR);
            break;
        }
        if (event_iface_snapshot & IFACE_EVENT_INTERFACE_DISCONNECTED)
        {
            active_iface->stop_rx(active_iface->context);
            sys_state_change(COMM_SYS_STATE_READY);
            comm_sys_notify_event(COMM_SYS_EVENT_DISCONNECTED);
            break;
        }
        if (sys_cmds & (COMM_SYS_CMD_STOP | COMM_SYS_CMD_RESET))
        {
            active_iface->stop_rx(active_iface->context);
            sys_state_change(COMM_SYS_STATE_READY);
            if (sys_cmds & COMM_SYS_CMD_RESET)
            {
                active_iface->reset(active_iface->context);
            }
        }
        break;

    case COMM_SYS_STATE_ERROR:
        if (sys_cmds & COMM_SYS_CMD_RESET)
        {
            active_iface->reset(active_iface->context);
            sys_state_change(COMM_SYS_STATE_READY);
        }
        if (event_iface_snapshot & IFACE_EVENT_INTERNAL_ERROR)
        {
            active_iface->deinit(active_iface->context);
            active_iface = NULL;
            sys_state_change(COMM_SYS_STATE_SELECTING);
        }
        break;

    case COMM_SYS_STATE_SELECTING:
    {
        comm_iface_t *new_iface = selector_get_active_iface();
        if (new_iface)
        {
            new_iface->init(new_iface->context);
            active_iface = new_iface;
            sys_state_change(COMM_SYS_STATE_READY);
            notify_iface_changed();
        }
        else
        {
            sys_state_change(COMM_SYS_STATE_DOWN);
            comm_sys_notify_event(COMM_SYS_EVENT_ERROR);
        }
        break;
    }
    }

    // --- Solicitud de cambio de interfaz (evento de sistema) ---
    comm_iface_id_t req = comm_sys_consume_pending_request();
    if (req < COMM_IFACE_MAX)
    {
        if (sys_state != COMM_SYS_STATE_SELECTING && sys_state != COMM_SYS_STATE_DOWN)
        {
            if (active_iface)
            {
                active_iface->deinit(active_iface->context);
                active_iface = NULL;
            }
            sys_state_change(COMM_SYS_STATE_SELECTING);
        }
    }
}

void fsm_rx(void)
{
}

/*******************************************************************************
 * Maquina de transmisión
 *
 *******************************************************************************/
static void send_response_to_interface(const char *resp, void *context)
{
    (void)resp;
    (void)context;
    // TODO: implementar cuando la FSM de TX esté operativa
}
void fsm_tx(void)
{
    // comm_buffer_tx_put(COMM_IFACE_USART, (const uint8_t *)resp, strlen(resp));
}

void task_comm(void)
{
    fsm_gestion();
    fsm_rx();
    fsm_tx();
}
