#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Identificadores de interfaces de comunicación.
 *
 * Enumera todas las interfaces de comunicación soportadas en el sistema.
 * Cada tipo de interfaz tiene buffers RX/TX dedicados gestionados por el módulo comm_buffers.
 */
typedef enum
{
    COMM_IFACE_USART = 0, /**< Interfaz serial USART (UART sobre RS-232/TTL) */
    COMM_IFACE_TCP,       /**< Interfaz de red TCP/IP */
    COMM_IFACE_USB,       /**< Interfaz USB CDC (puerto COM virtual) */
    COMM_IFACE_MAX        /**< Valor centinela para verificación de límites de arreglo */
} comm_interface_id_t;

typedef enum {
    COMM_STATE_NONE      = 0x00,
    COMM_STATE_ERROR     = 0x01,
    COMM_STATE_RX_ACTIVE = 0x02,
    COMM_STATE_TX_BUSY   = 0x04
} comm_interface_state_t;

#define COMM_STATE_MASK_ERROR     0x01
#define COMM_STATE_MASK_RX_ACTIVE 0x02
#define COMM_STATE_MASK_TX_BUSY   0x04

/**
 * @brief Declaración anticipada de la estructura de interfaz de comunicación.
 */
typedef struct comm_interface comm_interface_t;

/**
 * @brief Puntero de función para callback de indicación de recepción.
 *
 * Se invoca cuando se reciben datos en la interfaz especificada.
 *
 * @param iface_id  Identificador de la interfaz que recibió datos.
 * @param len       Cantidad de bytes recibidos y disponibles en el buffer RX.
 */
typedef void (*comm_rx_indication_callback_t)(comm_interface_id_t iface_id, size_t len);

/**
 * @brief Puntero de función para callback de transmisión completada.
 *
 * Se invoca cuando se completa una transmisión en la interfaz especificada.
 *
 * @param iface_id  Identificador de la interfaz que completó la transmisión.
 */
typedef void (*comm_tx_complete_callback_t)(comm_interface_id_t iface_id);

/**
 * @brief Estructura de interfaz de comunicación.
 *
 * Representación abstracta de una interfaz de comunicación. Cada interfaz física
 * (USART, TCP, USB) debe implementar esta estructura y registrarla usando
 * comm_register_interface() para participar en el sistema de comunicación.
 */
struct comm_interface
{
    comm_interface_id_t id;              /**< Identificador único para esta interfaz */
    void *context;                      /**< Puntero de contexto privado para la implementación de la interfaz */
    const char *name;                   /**< Nombre legible por humanos de la interfaz (ej: "USART2", "TCP0") */
    comm_interface_state_t state;       /**< Estado actual de la interfaz (bitmask) */
    
    bool (*send)(const uint8_t *data, size_t len);
    bool (*is_connected)(void *context);
    void (*start_rx)(void *context);       /**< Iniciar recepción (habilita DMA/interrupts) */
    void (*stop_rx)(void *context);        /**< Detener recepción */
    bool (*is_tx_ready)(void *context);    /**< Verificar si interfaz lista para TX */
    bool (*start_tx)(void *context);       /**< Transmitir desde buffer del sistema, devuelve true si exitoso */
    
    comm_rx_indication_callback_t rx_indication_cb;   /**< Callback para notificación de datos entrantes */
    comm_tx_complete_callback_t tx_complete_cb;       /**< Callback para completación de transmisión */
};

/**
 * @brief Registrar una interfaz de comunicación.
 *
 * Agrega una interfaz al registro global, haciéndola disponible para uso.
 * Debe llamarse antes de que pueda ocurrir cualquier comunicación en la interfaz.
 *
 * @param iface  Puntero a la estructura de interfaz a registrar.
 */
void comm_register_interface(comm_interface_t *iface);

/**
 * @brief Desregistrar una interfaz de comunicación.
 *
 * Elimina una interfaz del registro global.
 *
 * @param iface  Puntero a la estructura de interfaz a desregistrar.
 */
void comm_unregister_interface(comm_interface_t *iface);

/**
 * @brief Obtener una interfaz de comunicación registrada.
 *
 * @param id  El identificador de interfaz a buscar.
 * @return Puntero a la estructura de interfaz, o NULL si no se encontró o el ID es inválido.
 */
comm_interface_t *comm_get_interface(comm_interface_id_t id);

/**
 * @brief Iniciar recepción en la interfaz especificada.
 * @param id Identificador de la interfaz.
 */
void comm_interface_start_rx(comm_interface_id_t id);

/**
 * @brief Detener recepción en la interfaz especificada.
 * @param id Identificador de la interfaz.
 */
void comm_interface_stop_rx(comm_interface_id_t id);

/**
 * @brief Verificar si la interfaz está lista para transmitir.
 * @param id Identificador de la interfaz.
 * @return true si puede iniciar transmisión, false en caso contrario.
 */
bool comm_interface_is_tx_ready(comm_interface_id_t id);

/**
 * @brief Iniciar transmisión desde el buffer del sistema.
 * @param id Identificador de la interfaz.
 * @return true si se inició la transmisión, false si falló.
 */
bool comm_interface_start_tx(comm_interface_id_t id);

/**
 * @brief Enviar datos directamente a una interfaz (encolado en buffer TX).
 * @param id Identificador de la interfaz.
 * @param data Puntero a los datos.
 * @param len Cantidad de bytes.
 * @return true si se encoló correctamente.
 */
bool comm_interface_send(comm_interface_id_t id, const uint8_t *data, size_t len);

/**
 * @brief Comprobar si la recepción está activa en la interfaz.
 * @param id Identificador de la interfaz.
 * @return true si RX está activo.
 */
bool comm_interface_is_rx_active(comm_interface_id_t id);

/**
 * @brief Comprobar si la transmisión está ocupada.
 * @param id Identificador de la interfaz.
 * @return true si TX está ocupado.
 */
bool comm_interface_is_tx_busy(comm_interface_id_t id);

/**
 * @brief Comprobar si hay error en la interfaz.
 * @param id Identificador de la interfaz.
 * @return true si hay error.
 */
bool comm_interface_has_error(comm_interface_id_t id);

/**
 * @brief Establecer el estado de recepción activo.
 * @param id Identificador de la interfaz.
 * @param active true para activar RX, false para desactivar.
 */
void comm_interface_set_rx_active(comm_interface_id_t id, bool active);

/**
 * @brief Establecer el estado de transmisión ocupada.
 * @param id Identificador de la interfaz.
 * @param busy true para marcar TX como ocupado, false para liberar.
 */
void comm_interface_set_tx_busy(comm_interface_id_t id, bool busy);

/**
 * @brief Establecer el estado de error.
 * @param id Identificador de la interfaz.
 * @param error true para marcar error, false para limpiar.
 */
void comm_interface_set_error(comm_interface_id_t id, bool error);

/**
 * @brief Establecer el estado de una interfaz (API deprecada, usa setters específicos).
 * @param id Identificador de la interfaz.
 * @param state Nuevo estado.
 */
void comm_interface_set_state(comm_interface_id_t id, comm_interface_state_t state);

/**
 * @brief Obtener el estado actual de una interfaz.
 * @param id Identificador de la interfaz.
 * @return Estado actual combinado de bits.
 */
comm_interface_state_t comm_interface_get_state(comm_interface_id_t id);

/**
 * @brief Reiniciar una interfaz (detener RX, limpiar buffers, reiniciar RX).
 * @param id Identificador de la interfaz.
 */
void comm_interface_reset(comm_interface_id_t id);

#endif