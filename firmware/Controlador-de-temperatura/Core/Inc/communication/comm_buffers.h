#ifndef COMM_BUFFERS_H
#define COMM_BUFFERS_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "services/circular_buffer.h"
#include "communication/comm_interface.h"

/**
 * @brief Tamaño del buffer RX en bytes.
 * 
 * Tamaño del buffer circular para datos entrantes por interfaz.
 */
#define COMM_BUFFER_RX_SIZE 256

/**
 * @brief Tamaño del buffer TX en bytes.
 * 
 * Tamaño del buffer circular para datos salientes por interfaz.
 */
#define COMM_BUFFER_TX_SIZE 256

/**
 * @brief Estructura de metadatos del mensaje recibido.
 * 
 * Contiene información sobre datos recibidos para su procesamiento.
 */
typedef struct {
    comm_iface_id_t iface_id;  /**< Identificador de la interfaz origen */
    size_t len;                   /**< Longitud de los datos recibidos */
} comm_rx_message_t;

/**
 * @brief Estructura API de buffers de comunicación.
 * 
 * Proporciona interfaz para leer mensajes recibidos y consultar datos TX pendientes.
 */
typedef struct {
    bool (*read_msg)(comm_rx_message_t *msg);              /**< Callback para leer un mensaje recibido */
    size_t (*get_pending_tx_len)(comm_iface_id_t iface_id); /**< Consultar bytes TX pendientes */
} comm_buffers_api_t;

/**
 * @brief Inicializar buffers de comunicación.
 * 
 * Asigna e inicializa los buffers RX/TX circulares para todos los tipos de interfaz.
 * Debe llamarse antes de cualquier operación comm_buffer_*.
 */
void comm_buffers_init(void);

/**
 * @brief Colocar un byte recibido en el buffer RX.
 * 
 * @param iface_id  Identificador de la interfaz destino.
 * @param data      Byte recibido a almacenar.
 * @return true si el byte se almacenó, false si el buffer está lleno o el ID es inválido.
 */
bool comm_buffer_rx_put(comm_iface_id_t iface_id, uint8_t data);

/**
 * @brief Colocar datos en el buffer TX para transmisión.
 * 
 * @param iface_id  Identificador de la interfaz destino.
 * @param data      Puntero a los datos a encolar.
 * @param len       Cantidad de bytes a encolar.
 * @return true si todos los datos se encolaron, false si el buffer está lleno o el ID es inválido.
 */
bool comm_buffer_tx_put(comm_iface_id_t iface_id, const uint8_t *data, size_t len);

/**
 * @brief Obtener datos del buffer RX.
 * 
 * @param iface_id  Identificador de la interfaz origen.
 * @param data      Buffer donde almacenar los datos recibidos.
 * @param len       Puntero a bytes máximos a leer, actualizado con bytes reales leídos.
 * @return true si se obtuvieron datos, false si el buffer está vacío o el ID es inválido.
 */
bool comm_buffer_rx_get(comm_iface_id_t iface_id, uint8_t *data, size_t *len);

/**
 * @brief Obtener datos del buffer TX para transmisión real.
 * 
 * @param iface_id  Identificador de la interfaz origen.
 * @param data      Buffer donde almacenar los datos para transmisión.
 * @param len       Puntero a bytes máximos a leer, actualizado con bytes reales leídos.
 * @return true si se obtuvieron datos, false si el buffer está vacío o el ID es inválido.
 */
bool comm_buffer_tx_get(comm_iface_id_t iface_id, uint8_t *data, size_t *len);

/**
 * @brief Obtener cantidad de bytes disponibles en buffer RX.
 * 
 * @param iface_id  Identificador de la interfaz.
 * @return Cantidad de bytes disponibles para leer, o 0 si el ID es inválido.
 */
size_t comm_buffer_rx_count(comm_iface_id_t iface_id);

/**
 * @brief Obtener cantidad de bytes pendientes en buffer TX.
 * 
 * @param iface_id  Identificador de la interfaz.
 * @return Cantidad de bytes pendientes para transmitir, o 0 si el ID es inválido.
 */
size_t comm_buffer_tx_count(comm_iface_id_t iface_id);

/**
  * @brief Limpiar todos los datos del buffer RX.
  * 
  * @param iface_id  Identificador de la interfaz.
  */
void comm_buffer_rx_clear(comm_iface_id_t iface_id);

/**
  * @brief Prepend data to the front of TX buffer.
  * 
  * @param iface_id  Identificador de la interfaz.
  * @param data      Puntero a los datos a insertar al frente.
  * @param len       Cantidad de bytes a insertar.
  * @return true si todos los datos se insertaron, false si el buffer está lleno o el ID es inválido.
  */
bool comm_buffer_tx_prepend(comm_iface_id_t iface_id, const uint8_t *data, size_t len);

#endif