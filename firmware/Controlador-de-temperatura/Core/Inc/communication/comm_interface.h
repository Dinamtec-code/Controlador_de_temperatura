#ifndef COMM_INTERFACE_H
#define COMM_INTERFACE_H

#include "services/circular_buffer.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

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
 * @brief Identificadores de interfaces de comunicación.
 *
 * Enumera todas las interfaces de comunicación soportadas en el sistema.
 * Cada tipo de interfaz tiene buffers RX/TX dedicados gestionados por el módulo comm_buffers.
 */
typedef enum
{
  COMM_IFACE_USART = 0, /**< Interfaz serial USART (UART sobre RS-232/TTL) */
  // COMM_IFACE_TCP,       /**< Interfaz de red TCP/IP */
  // COMM_IFACE_USB,       /**< Interfaz USB CDC (puerto COM virtual) */
  COMM_IFACE_MAX /**< Valor centinela para verificación de límites de arreglo */
} comm_iface_id_t;

typedef enum
{
  COMM_STATE_NONE = 0x00,
  COMM_STATE_CONNECTED = 0x01,
  COMM_STATE_RX_ACTIVE = 0x02,
  COMM_STATE_TX_ACTIVE = 0x04
} comm_iface_state_t;

typedef enum
{
  IFACE_EVENT_RX_DATA_AVAILABLE = 0x0,       /**< Datos recibidos e informacion del buffer de entrada actualkizada */
  IFACE_EVENT_TX_COMPLETE = 0x01,            /**< Transmision por DMA finalizada */
  IFACE_EVENT_INTERFACE_CONNECTED = 0x02,    /**< La interfaz se conecto con el periferico */
  IFACE_EVENT_INTERFACE_DISCONNECTED = 0x04, /**< La interfaz se desconecto del periferico */
  IFACE_EVENT_ERROR = 0x08                   /**< Error de hardware detectado en el periferico */
} comm_iface_event_t;

/**
 * @brief Declaración anticipada de la estructura de interfaz de comunicación.
 */
typedef struct comm_iface comm_iface_t;

/**
 *
 */
typedef void (*comm_iface_sink_t)(const comm_iface_event_t *evt);
typedef void (*comm_iface_post_event)(comm_iface_event_t *evt);
typedef void (*comm_iface_get_event)(comm_iface_event_t *evt);

/**
 * @brief Estructura de interfaz de comunicación.
 *
 * Representación abstracta de una interfaz de comunicación. Cada interfaz física
 * (USART, TCP, USB) debe implementar esta estructura y registrarla usando
 * comm_register_interface() para participar en el sistema de comunicación.
 */
struct comm_iface
{
  comm_iface_id_t id;       /**< Identificador único para esta interfaz */
  void *context;            /**< Puntero de contexto privado para la implementación de la interfaz */
  const char *name;         /**< Nombre legible de la interfaz (ej: "USART2", "TCP0") */
  comm_iface_state_t state; /**< Estado actual de la interfaz (bitmask) */
  comm_iface_event_t event; /**< Eventos producidos por el hardware */
  uint8_t *rx_buffer;
  uint8_t *tx_buffer;
  size_t rx_len;
  size_t tx_len;
  int (*init)(void *ctx);
  int (*deinit)(void *ctx);
  void (*start_rx)(void *context);      /**< Iniciar recepción (habilita DMA/interrupt IDLE) */
  void (*stop_rx)(void *context);       /**< Detener recepción */
  void (*start_tx)(void *context);      /**< Transmitir desde buffer del sistema */
  void (*restart)(void *context);       /**< Verificar si interfaz lista para TX */
  comm_iface_event_t comm_iface_sink_t; /**< Callback para notificación de datos entrantes */
  comm_iface_post_event post_event;
  comm_iface_get_event get_event;
};

/**
 * @brief Registrar una interfaz de comunicación.
 *
 * Agrega una interfaz al registro global, haciéndola disponible para uso.
 * Debe llamarse antes de que pueda ocurrir cualquier comunicación en la interfaz.
 *
 * @param iface  Puntero a la estructura de interfaz a registrar.
 */
void comm_register_interface(comm_iface_t *iface);

/**
 * @brief Desregistrar una interfaz de comunicación.
 *
 * Elimina una interfaz del registro global.
 *
 * @param iface  Puntero a la estructura de interfaz a desregistrar.
 */
void comm_unregister_interface(comm_iface_t *iface);

/**
 * @brief Obtener una interfaz de comunicación registrada.
 *
 * @param id  El identificador de interfaz a buscar.
 * @return Puntero a la estructura de interfaz, o NULL si no se encontró o el ID es inválido.
 */
comm_iface_id_t *comm_get_interface(comm_iface_id_t id);

/**
 * @brief Iniciar recepción en la interfaz especificada.
 * @param id Identificador de la interfaz.
 */
void comm_iface_start_rx(comm_iface_id_t id);

/**
 * @brief Detener recepción en la interfaz especificada.
 * @param id Identificador de la interfaz.
 */
void comm_iface_stop_rx(comm_iface_id_t id);

/**
 * @brief Iniciar transmisión desde el buffer del sistema.
 * @param id Identificador de la interfaz.
 * @return true si se inició la transmisión, false si falló.
 */

void comm_iface_start_tx(comm_iface_id_t id);
/**
 * @brief Reiniciar una interfaz (detener RX, limpiar buffers, reiniciar RX).
 * @param id Identificador de la interfaz.
 */
void comm_iface_restart(comm_iface_id_t id);

/**
 * @brief Obtener el estado actual de una interfaz.
 * @param id Identificador de la interfaz.
 * @return Estado actual combinado de bits.
 */
comm_iface_state_t comm_interface_get_state(comm_iface_id_t id);

/**
 * @brief Establecer el estado de respuesta pendiente.
 * @param id Identificador de la interfaz.
 * @param pending true para marcar respuesta pendiente, false para limpiar.
 */
void comm_interface_set_response_pending(comm_iface_id_t id, bool pending);

/**
 * @brief Comprobar si hay respuesta pendiente.
 * @param id Identificador de la interfaz.
 * @return true si hay respuesta pendiente.
 */
bool comm_interface_is_response_pending(comm_iface_id_t id);

#endif