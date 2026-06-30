#include "tasks/task_comm.h"
#include "system/config_manager.h"

#include "communication/comm_driver_api.h"
#include "communication/comm_sys_api.h"
#include "communication/app_msg_api.h"
#include "communication/comm_iface_registry.h"

#include "services/circular_buffer.h"
#include "services/error_handler.h"

#include "main.h"
#include <stdio.h>
#include <string.h>

/*******************************************************************************
 * Comando interno para la FSM (no expuesto en API pública)
 * CMD_DISABLE_RX: Detiene recepción sin desinicializar la interfaz
 ******************************************************************************/
#define COMM_SYS_CMD_DISABLE_RX (1u << 3)

/*******************************************************************************
 * Registros del sistema
 ******************************************************************************/
static comm_sys_state_t sys_state = COMM_SYS_STATE_DOWN;

/*******************************************************************************
 * Registros de la interfaz
 ******************************************************************************/

static comm_iface_t *active_iface = NULL;
static comm_iface_event_t event_iface_snapshot = IFACE_EVENT_NONE;

/*******************************************************************************
 * Buffers de transporte (propiedad de Communication Task)
 ******************************************************************************/

static uint8_t rx_buffer_mem[RX_BUFFER_SIZE];
static cb_t rx_buffer = {
    .buffer = rx_buffer_mem,
    .size = RX_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
};

static uint8_t tx_buffer_mem[TX_BUFFER_SIZE];
static cb_t tx_buffer = {
    .buffer = tx_buffer_mem,
    .size = TX_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
};

/*******************************************************************************
 * Evento interno para solicitud de transmisión desde la aplicación
 * ============================================================================
 * Se reutiliza IFACE_EVENT_TX_COMPLETE como señal de software para evitar
 * agregar un tipo nuevo. El driver hardware no usa este evento como señal.
 ******************************************************************************/
static comm_iface_event_t internal_events = IFACE_EVENT_NONE;
static volatile bool tx_pending_atomic = false;

static inline void set_tx_request(void)
{
    __disable_irq();
    internal_events |= IFACE_EVENT_TX_COMPLETE;
    __enable_irq();
}

static inline bool check_and_clear_tx_request(void)
{
    __disable_irq();
    bool has_request = (internal_events & IFACE_EVENT_TX_COMPLETE) != 0;
    if (has_request)
    {
        internal_events &= ~IFACE_EVENT_TX_COMPLETE;
    }
    __enable_irq();
    return has_request;
}

/*******************************************************************************
 * API de acceso a buffers
 ******************************************************************************/

comm_iface_t *comm_task_get_active_iface(void)
{
    return active_iface;
}

void comm_task_get_buffers(cb_t **rx_ptr, cb_t **tx_ptr)
{
    *rx_ptr = &rx_buffer;
    *tx_ptr = &tx_buffer;
}

comm_sys_state_t comm_task_get_sys_state(void)
{
    return sys_state;
}

/*******************************************************************************
 * FSM DE GESTIÓN DE INTERFAZ
 * Estados: DOWN, READY, ACTIVE, ERROR, SELECTING
 ******************************************************************************/

static comm_iface_t *selector_get_active_iface(void)
{
    comm_iface_id_t req = comm_sys_consume_pending_request();
    if (req < COMM_IFACE_MAX)
    {
        comm_iface_t *iface = comm_get_iface(req);
        if (iface)
            return iface;
    }

    comm_iface_id_t pref = config_get_preferred_iface();
    comm_iface_t *iface = comm_get_iface(pref);
    if (iface)
        return iface;

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
    comm_sys_cmd_t sys_cmds = comm_sys_consume_commands();

    if (active_iface)
    {
        event_iface_snapshot = active_iface->get_event(active_iface->context);
    }

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
        if (sys_cmds & COMM_SYS_CMD_DISABLE_RX)
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

/*******************************************************************************
 * Interfaz de aplicación registrada
 ******************************************************************************/

static const app_msg_iface_t *app_iface = NULL;

void comm_task_register_app_iface(const app_msg_iface_t *iface)
{
    app_iface = iface;
}

/*******************************************************************************
 * Maquina de recepción
 * Estados: RX_IDLE, RX_GATHERING
 * Comportamiento SCPI: APP_MSG_BUSY genera query interrupt (mensaje cancelado)
 ******************************************************************************/

void fsm_rx(void)
{
    if (!active_iface || !app_iface)
        return;

    static enum { RX_IDLE,
                  RX_GATHERING } state = RX_IDLE;
    static uint8_t msg_buffer[RX_BUFFER_SIZE];
    static size_t msg_len = 0;

    switch (state)
    {
    case RX_IDLE:
        if (event_iface_snapshot & IFACE_EVENT_RX_DATA_AVAILABLE)
        {
            active_iface->protect_rx(active_iface->context);
            state = RX_GATHERING;
        }
        else
        {
            break;
        }
        /* falls through */
    case RX_GATHERING:
    {
        bool msg_completed = false;
        uint8_t byte;

        while (cb_get(active_iface->rx_buffer, &byte) == BUF_OK)
        {
            if (byte == '\n')
            {
                msg_completed = true;
                if (msg_len < sizeof(msg_buffer))
                {
                    msg_buffer[msg_len++] = byte;
                }
                break;
            }
            else
            {
                if (msg_len < sizeof(msg_buffer))
                {
                    msg_buffer[msg_len++] = byte;
                }
                else
                {
                    break; /* Buffer de mensaje overflow - se cancela el mensaje */
                }
            }
        }

        if (msg_completed)
        {
            app_msg_response_t resp = app_iface->on_message_ready(
                app_iface->context, msg_buffer, msg_len);

            if (resp == APP_MSG_OK)
            {
                msg_len = 0;
            }
            else if (resp == APP_MSG_BUSY)
            {
                /* SCPI Query Interrupt: cancelamos el mensaje actual */
                msg_len = 0;
                app_iface->on_error(app_iface->context, APP_MSG_ERROR_INTERNAL);
            }

            if (cb_status(active_iface->rx_buffer) == BUF_EMPTY)
            {
                state = RX_IDLE;
            }
            else
            {
                state = RX_GATHERING;
            }
        }
        else
        {
            if (cb_status(active_iface->rx_buffer) == BUF_EMPTY)
            {
                state = RX_IDLE;
            }
            else
            {
                state = RX_GATHERING;
            }
        }

        active_iface->unprotect_rx(active_iface->context);
        break;
    }
    }
}

/*******************************************************************************
 * Maquina de transmisión
 * Estados: TX_IDLE, TX_BUSY
 ******************************************************************************/

void fsm_tx(void)
{
    if (!active_iface || !app_iface)
        return;

    static enum { TX_IDLE,
                  TX_BUSY } state = TX_IDLE;

    switch (state)
    {
    case TX_IDLE:
    {
        bool has_pending = check_and_clear_tx_request();
        if (!has_pending && !(event_iface_snapshot & IFACE_EVENT_TX_COMPLETE))
        {
            break;
        }

        comm_response_t resp = active_iface->start_tx(active_iface->context);
        switch (resp)
        {
        case COMM_IFACE_OK:
            __disable_irq();
            tx_pending_atomic = false;
            __enable_irq();
            state = TX_BUSY;
            break;
        case COMM_IFACE_BUSY:
            break;
        case COMM_IFACE_IDLE:
            __disable_irq();
            tx_pending_atomic = false;
            __enable_irq();
            break;
        case COMM_IFACE_ERROR:
            app_iface->on_error(app_iface->context, APP_MSG_ERROR_TX);
            __disable_irq();
            tx_pending_atomic = false;
            __enable_irq();
            break;
        }
        break;
    }

    case TX_BUSY:
        if (event_iface_snapshot & IFACE_EVENT_TX_COMPLETE)
        {
            state = TX_IDLE;
            app_iface->on_tx_done(app_iface->context);
        }
        else if (event_iface_snapshot & (IFACE_EVENT_TX_ERROR_TIMEOUT |
                                        IFACE_EVENT_TX_ERROR_BUS_FAULT))
        {
            state = TX_IDLE;
            app_iface->on_error(app_iface->context, APP_MSG_ERROR_TX);
        }
        break;
    }
}

/*******************************************************************************
 * FUNCIÓN PARA SOLICITAR TRANSMISIÓN DESDE LA APLICACIÓN
 ******************************************************************************/

void comm_app_send_response(const uint8_t *data, size_t len)
{
    if (!active_iface || !active_iface->tx_buffer)
        return;

    active_iface->protect_tx(active_iface->context);
    for (size_t i = 0; i < len; i++)
    {
        if (cb_put(active_iface->tx_buffer, data[i]) != BUF_OK)
        {
            break;
        }
    }
    active_iface->unprotect_tx(active_iface->context);

    __disable_irq();
    tx_pending_atomic = true;
    internal_events |= IFACE_EVENT_TX_COMPLETE;
    __enable_irq();
}

void task_comm(void)
{
    fsm_gestion();
    fsm_rx();
    fsm_tx();
}