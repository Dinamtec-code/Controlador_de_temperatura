#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include "services/circular_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Códigos de error específicos del subsistema de comunicación.
 */
typedef enum
{
    COMM_ERR_NONE,
    COMM_ERR_UART_OVERRUN,
    COMM_ERR_UART_FRAMING,
    COMM_ERR_UART_PARITY,
    COMM_ERR_DMA,
    COMM_ERR_TIMEOUT,
    COMM_ERR_PROTOCOL,
    COMM_ERR_INTERNAL
} comm_error_t;

/**
 * @brief Identificadores únicos de las interfaces de comunicación soportadas.
 */
typedef enum
{
    COMM_IFACE_USART = 0,   /**< Interfaz serial USART (UART sobre RS-232/TTL) */
    COMM_IFACE_TCP,         /**< Interfaz de red TCP/IP */
    COMM_IFACE_USBTMC,      /**< Interfaz USB TMC / CDC (puerto COM virtual) */
    COMM_IFACE_MAX          /**< Valor centinela para verificación de límites */
} comm_iface_id_t;

/**
 * @brief Máscara de bits para representar el estado operativo de una interfaz.
 */
typedef enum
{
    COMM_STATE_NONE       = 0x00,
    COMM_STATE_CONNECTED  = (1u << 0), /**< Enlace físico/lógico establecido */
    COMM_STATE_RX_ACTIVE  = (1u << 1), /**< Hay datos ingresando o buffer con datos */
    COMM_STATE_TX_ACTIVE  = (1u << 2), /**< El periférico/DMA está transmitiendo */
    COMM_STATE_ERROR      = (1u << 3)  /**< La interfaz está en un estado de falla global */
} comm_iface_state_t;

/**
 * @brief Eventos asincrónicos producidos por el hardware de comunicaciones.
 */
typedef enum
{
    /* --- Eventos de Flujo Normal --- */
    IFACE_EVENT_NONE                    = 0x00,
    IFACE_EVENT_INTERFACE_CONNECTED     = (1u << 0),
    IFACE_EVENT_INTERFACE_DISCONNECTED  = (1u << 1),
    IFACE_EVENT_RX_DATA_AVAILABLE       = (1u << 2),
    IFACE_EVENT_TX_COMPLETE             = (1u << 3),

    /* --- Eventos de Error en Recepción (RX) --- */
    IFACE_EVENT_RX_ERROR_OVERFLOW       = (1u << 4), /**< Overflow por software o hardware */
    IFACE_EVENT_RX_ERROR_FRAMING        = (1u << 5), /**< Error de encuadre (UART) */
    IFACE_EVENT_RX_ERROR_PARITY         = (1u << 6), /**< Error de paridad (UART) */

    /* --- Eventos de Error en Transmisión (TX) --- */
    IFACE_EVENT_TX_ERROR_TIMEOUT        = (1u << 7), /**< El host no consumió los datos a tiempo */
    IFACE_EVENT_TX_ERROR_BUS_FAULT      = (1u << 8), /**< Error físico (ej: DMA Bus Error) */
    
    /* --- Evento General de Mantenimiento --- */
    IFACE_EVENT_INTERNAL_ERROR          = (1u << 9)  /**< Falla de asignación de memoria, etc. */
} comm_iface_event_t;

/* Declaración anticipada de la estructura */
typedef struct comm_iface comm_iface_t;

/**
 * @brief Estructura de interfaz de comunicación (Contrato Abstracto).
 */
struct comm_iface
{
    comm_iface_id_t id;         /**< Identificador único para esta interfaz */
    void *context;              /**< Puntero de contexto privado para la implementación */
    const char *name;           /**< Nombre legible de la interfaz (ej: "USART2") */
    comm_iface_state_t state;   /**< Estado actual de la interfaz (bitmask) */
    comm_iface_event_t event;   /**< Eventos pendientes producidos por el hardware */

    circular_buffer_t *rx_buffer; /**< Inyección del buffer de recepción circular */
    uint8_t *tx_buffer;           /**< Inyección del buffer de transmisión lineal */
    size_t tx_len;                /**< Tamaño máximo asignado al buffer de transmisión */

    /* --- Callbacks de Ciclo de Vida del Periférico --- */
    comm_error_t (*init)(void *ctx);
    void (*deinit)(void *ctx);
    comm_error_t (*reset)(void *ctx);

    /* --- Callbacks de Control de Flujo (I/O) --- */
    void (*start_rx)(void *ctx);  
    void (*stop_rx)(void *ctx);   
    bool (*start_tx)(void *ctx);  /**< Modificado a bool para validar disparo del DMA */

    /* --- Callbacks de Gestión de Eventos --- */
    void (*set_event)(void *ctx, comm_iface_event_t event);
    comm_iface_event_t (*get_event)(void *ctx);

    /* --- Callbacks de Protección de Memoria (Secciones Críticas) --- */
    void (*protect_rx)(void);
    void (*unprotect_rx)(void);
    void (*protect_tx)(void);
    void (*unprotect_tx)(void);
};

/* ========================================================================== */
/* API Global del Subsistema de Gestión de Interfaces            */
/* ========================================================================== */

/**
 * @brief Agrega una interfaz al registro global haciéndola disponible para el sistema.
 */
void comm_register_interface(comm_iface_t *iface);

/**
 * @brief Elimina una interfaz del registro global.
 */
void comm_unregister_interface(comm_iface_t *iface);

/**
 * @brief Obtiene el puntero a una interfaz registrada a partir de su ID.
 * @return Puntero a la estructura de la interfaz, o NULL si no está registrada.
 */
comm_iface_t *comm_get_interface(comm_iface_id_t id); // <-- Corregido el tipo de retorno

/**
 * @brief Iniciar recepción por hardware en la interfaz especificada.
 */
void comm_iface_start_rx(comm_iface_id_t id);

/**
 * @brief Detener recepción por hardware en la interfaz especificada.
 */
void comm_iface_stop_rx(comm_iface_id_t id);

/**
 * @brief Intenta iniciar la transmisión de datos pendientes desde el buffer del sistema.
 * @return true si el hardware aceptó el bloque y comenzó la transmisión DMA.
 */
bool comm_iface_start_tx(comm_iface_id_t id); // <-- Corregido a bool

/**
 * @brief Reinicia por completo el canal de comunicación (Detiene RX, limpia buffers y re-activa).
 */
void comm_iface_restart(comm_iface_id_t id);

/**
 * @brief Obtiene la máscara de bits con el estado consolidado de la interfaz.
 */
comm_iface_state_t comm_iface_get_state(comm_iface_id_t id); // <-- Convención de nombre corregida

#endif /* COMM_INTERFACE_H */