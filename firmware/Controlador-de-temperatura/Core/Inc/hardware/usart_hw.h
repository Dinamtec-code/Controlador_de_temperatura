#ifndef USART_HW_H
#define USART_HW_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "main.h"
#include "services/circular_buffer.h"
#include "communication/comm_interface.h"

/**
 * @brief Handle HAL de USART2.
 *
 * Referencia al handle UART de USART2 configurado y administrado por el driver.
 */
extern UART_HandleTypeDef huart2;

/* --- API Pública de Inicialización y Registro --- */

/**
 * @brief Inicializa el objeto abstracto de interfaz, inyecta los buffers y lo registra.
 *
 * Configura la estructura estática interna de la interfaz de comunicación, vincula
 * las callbacks de bajo nivel del periférico y da de alta el canal en el gestor global.
 *
 * @param dma_rx_cb_buffer Puntero al buffer circular de software para la recepción.
 * @param dma_tx_buffer    Puntero al bloque de memoria lineal asignado para la transmisión DMA.
 * @param tx_len           Capacidad máxima en bytes del buffer de transmisión física.
 */
void usart_iface_register(circular_buffer_t *dma_rx_cb_buffer, uint8_t *dma_tx_buffer, size_t tx_len);

/* --- API de Control de Flujo (Callbacks mapeadas a comm_iface_t) --- */

/**
 * @brief Iniciar recepción por hardware en el periférico USART2.
 *
 * Dispara el mecanismo de recepción por DMA acoplado al evento RxEventToIdle (IDLE Line).
 * Muta el flag de estado de la interfaz a COMM_STATE_RX_ACTIVE.
 * * @param context Puntero de contexto privado (puede ser NULL).
 */
void usart_hw_start_rx(void *context);

/**
 * @brief Detener recepción por hardware en el periférico USART2.
 *
 * Apaga el canal DMA de recepción y limpia los flags correspondientes en la interfaz.
 * * @param context Puntero de contexto privado (puede ser NULL).
 */
void usart_hw_stop_rx(void *context);

/**
 * @brief Extrae datos del buffer del sistema e inicia la transferencia física por DMA.
 *
 * Verifica la disponibilidad del hardware de la UART y consulta al buffer circular
 * de transmisión del sistema si existen datos listos para despacho.
 *
 * @param context Puntero de contexto privado (puede ser NULL).
 * @return true si la transferencia por DMA inició con éxito; false si el hardware 
 * está ocupado o no había datos disponibles en el buffer.
 */
bool usart_hw_start_tx(void *context);

/* --- API de Gestión de Eventos Asincrónicos (Thread-Safe) --- */

/**
 * @brief Registra un flag de evento asincrónico en la interfaz.
 * * @param ctx   Puntero de contexto privado (puede ser NULL).
 * @param event Flag o bitmask de evento a encender (ej: IFACE_EVENT_RX_DATA_AVAILABLE).
 */
void usart_hw_set_event(void *ctx, comm_iface_event_t event);

/**
 * @brief Obtiene y limpia los eventos pendientes en la interfaz (Clear-on-Read).
 * * @param ctx Puntero de contexto privado (puede ser NULL).
 * @return Bitmask con todos los eventos acumulados desde la última lectura.
 */
comm_iface_event_t usart_hw_get_event(void *ctx);

#endif /* USART_HW_H */