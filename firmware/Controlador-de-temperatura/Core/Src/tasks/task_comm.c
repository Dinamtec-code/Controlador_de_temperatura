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
 * Registros del sistema
 *
 ******************************************************************************/
static comm_sys_state_t sys_state = COMM_SYS_STATE_DOWN;

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

static uint8_t rx_buffer_mem[RX_BUFFER_SIZE];
static cb_t rx_buffer = {
    .buffer = rx_buffer_mem,
    .size = RX_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
};

static uint8_t tx_buffer_mem[RX_BUFFER_SIZE];
static cb_t tx_buffer = {
    .buffer = tx_buffer_mem,
    .size = TX_BUFFER_SIZE,
    .head = 0,
    .tail = 0,
};

static volatile bool tx_pending = false;

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

/**
 * @brief Interfaz de aplicación registrada.
 *
 * Se espera que sea configurada externamente (por ejemplo, en la inicialización)
 * mediante una función comm_task_register_app_iface().
 */
static const app_msg_iface_t *app_iface = NULL;

void comm_task_register_app_iface(const app_msg_iface_t *iface)
{
    app_iface = iface;
}

/*******************************************************************************
 * Maquina de recepción
 *
 * Estados: RX_IDLE, RX_GATHERING
 ******************************************************************************/
void fsm_rx(void)
{
    if (!active_iface || !app_iface)
        return;

    static enum { RX_IDLE,
                  RX_GATHERING } state = RX_IDLE;
    static uint8_t msg_buffer[RX_BUFFER_SIZE]; // Buffer para ensamblar el mensaje
    static size_t msg_len = 0;

    switch (state)
    {
    case RX_IDLE:
        // Transición a GATHERING si hay datos disponibles
        if (event_iface_snapshot & IFACE_EVENT_RX_DATA_AVAILABLE)
        {
            active_iface->protect_rx(active_iface->context);
            state = RX_GATHERING;
            // NO hacemos break aquí: caemos intencionalmente al case RX_GATHERING
            // para procesar los datos en el mismo ciclo.
        }
        else
        {
            break; // Sin datos, salimos.
        }
        /* falls through */
    case RX_GATHERING:
    {
        bool msg_completed = false;
        uint8_t byte;

        // Procesar datos hasta que el buffer esté vacío o se complete un mensaje.
        while (cb_get(active_iface->rx_buffer, &byte) == BUF_OK)
        {
            // Ensamblar el mensaje. El carácter LF actúa como delimitador,
            // pero TODOS los bytes (incluyendo CR) se transfieren a la aplicación.
            if (byte == '\n')
            { // LF (0x0A) delimita el fin del mensaje
                msg_completed = true;
                // Incluimos el LF en el mensaje y se lo pasamos a la aplicación
                if (msg_len < sizeof(msg_buffer))
                {
                    msg_buffer[msg_len++] = byte;
                }
                break; // Salimos del bucle para limitar a un mensaje por ciclo
            }
            else
            {
                // Acumular cualquier otro byte (incluye CR, datos binarios, etc.)
                if (msg_len < sizeof(msg_buffer))
                {
                    msg_buffer[msg_len++] = byte;
                }
                else
                {
                    // Overflow del buffer de mensaje: deberíamos señalizar un error
                    // y descartar. Por ahora, simplemente rompemos el bucle.
                    break;
                }
            }
        }

        // Si se completó un mensaje, notificar a la aplicación
        if (msg_completed)
        {
            // Entregar el mensaje a la capa de aplicación
            app_msg_response_t resp = app_iface->on_message_ready(
                app_iface->context, msg_buffer, msg_len);

            if (resp == APP_MSG_OK)
            {
                // Aplicación procesó el mensaje. Reiniciamos el buffer de ensamblaje.
                msg_len = 0;
                // El índice de lectura del buffer circular ya fue avanzado por cb_get.
            }
            else
            {
                // Aplicación ocupada: no avanzamos el índice de lectura,
                // el mensaje se reintentará en el próximo ciclo.
                // NOTA: En una implementación real, necesitaríamos un mecanismo
                // para retroceder el índice de lectura (tail) del buffer circular.
                // Por ahora, asumimos que cb_get ya avanzó tail y no podemos retroceder.
                // Una solución más robusta requeriría un buffer de "vistazo previo".
            }

            // Evaluar el siguiente estado después de publicar el mensaje
            if (cb_status(active_iface->rx_buffer) == BUF_EMPTY)
            {
                state = RX_IDLE;
            }
            else
            {
                state = RX_GATHERING; // Permanecer en GATHERING para el próximo ciclo
            }
        }
        else
        {
            // No se completó ningún mensaje (porque no se recibió LF)
            if (cb_status(active_iface->rx_buffer) == BUF_EMPTY)
            {
                state = RX_IDLE;
            }
            else
            {
                state = RX_GATHERING; // Continuar en el próximo ciclo
            }
        }

        // Liberar la protección adquirida al entrar en GATHERING
        active_iface->unprotect_rx(active_iface->context);
        break;
    }
    } // fin del switch
}

/*******************************************************************************
 * Maquina de transmisión
 *
 * Estados: TX_IDLE, TX_BUSY
 *******************************************************************************/
void fsm_tx(void)
{
    if (!active_iface || !app_iface)
        return;

    static enum { TX_IDLE,
                  TX_BUSY } state = TX_IDLE;
    //static bool tx_pending = false; // Señal interna para iniciar transmisión

    switch (state)
    {
    case TX_IDLE:
        if (tx_pending || (event_iface_snapshot & IFACE_EVENT_TX_COMPLETE))
        {
            // Intentar iniciar una transmisión (ya sea nueva o tras completar una)
            comm_response_t resp = active_iface->start_tx(active_iface->context);
            switch (resp)
            {
            case COMM_IFACE_OK:
                tx_pending = false;
                state = TX_BUSY;
                break;
            case COMM_IFACE_BUSY:
                // Ocupado, reintentar en el siguiente ciclo
                break;
            case COMM_IFACE_IDLE:
                // Buffer vacío, sin datos que transmitir
                tx_pending = false;
                break;
            case COMM_IFACE_ERROR:
                // Error irrecuperable: notificar a la aplicación.
                // La FSM de Gestión se encargará de ejecutar reset() si es necesario.
                app_iface->on_error(app_iface->context, APP_MSG_ERROR_TX);
                tx_pending = false;
                break;
            }
        }
        break;

    case TX_BUSY:
        // Evaluar eventos de la interfaz desde el snapshot
        if (event_iface_snapshot & IFACE_EVENT_TX_COMPLETE)
        {
            state = TX_IDLE;
            app_iface->on_tx_done(app_iface->context);
        }
        else if (event_iface_snapshot & (IFACE_EVENT_TX_ERROR_TIMEOUT |
                                         IFACE_EVENT_TX_ERROR_BUS_FAULT))
        {
            // Error en la transmisión. Notificar a la aplicación.
            // La FSM de Gestión consumirá el error y llamará a reset() en el ciclo siguiente.
            state = TX_IDLE;
            app_iface->on_error(app_iface->context, APP_MSG_ERROR_TX);
        }
        else if (tx_pending)
        {
            // Hay una nueva solicitud de transmisión mientras la anterior está en curso.
            // Podemos encolarla (no implementado) o ignorarla por ahora.
            // Lo más seguro es mantener tx_pending = true y esperar.
        }
        break;
    }
}

// -----------------------------------------------------------------------------
// FUNCIÓN PARA SOLICITAR TRANSMISIÓN DESDE LA APLICACIÓN
// -----------------------------------------------------------------------------
void comm_app_send_response(const uint8_t *data, size_t len)
{
    if (!active_iface || !active_iface->tx_buffer)
        return;

    // Copiar datos al buffer de transmisión
    active_iface->protect_tx(active_iface->context);
    for (size_t i = 0; i < len; i++)
    {
        if (cb_put(active_iface->tx_buffer, data[i]) != BUF_OK)
        {
            break; // Buffer lleno, descartar el resto
        }
    }
    active_iface->unprotect_tx(active_iface->context);

    tx_pending = true;
}

void task_comm(void)
{
    fsm_gestion();
    fsm_rx();
    fsm_tx();
}
