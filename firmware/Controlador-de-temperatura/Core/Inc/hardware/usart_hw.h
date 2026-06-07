#ifndef USART_HW_H
#define USART_HW_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"

/**
 * @brief Handle HAL de USART2.
 *
 * Referencia al handle UART de USART2 configurado por CubeMX.
 */
extern UART_HandleTypeDef huart2;

/**
 * @brief Inicializar la interfaz hardware USART.
 *
 * Configura USART2 y registra la interfaz en el subsistema de comunicación.
 * Se llama durante el inicio del sistema.
 */
void usart_hw_init(void);

/**
 * @brief Iniciar recepción DMA en USART2.
 *
 * Comienza a escuchar datos entrantes usando DMA en modo circular.
 */
void usart_hw_start_rx(void *context);

/**
 * @brief Manejar interrupción IDLE de USART2.
 *
 * Procesa datos recibidos cuando se detecta línea IDLE, indicando
 * una pausa en la transmisión. Mueve los bytes recibidos al buffer de comunicación.
 */
void usart_hw_idle_handler(void);

/**
 * @brief Enviar una cadena null-terminada vía USART2.
 *
 * @param str  Cadena a transmitir (bloqueante, carácter por carácter).
 */
void usart_hw_send_str(const char *str);

/**
 * @brief Enviar un buffer raw vía USART2.
 *
 * @param data  Buffer que contiene los datos a transmitir.
 * @param len   Cantidad de bytes a transmitir.
 */
void usart_hw_send_buf(uint8_t *data, size_t len);

/**
 * @brief Verificar si USART2 está listo para transmisión.
 *
 * @return true si USART2 está en estado READY y puede aceptar nuevos datos.
 */
bool usart_hw_is_tx_ready(void *context);

/**
 * @brief Transmitir datos pendientes desde el buffer TX del sistema.
 *
 * Envía datos encolados en el buffer TX mediante DMA si el periférico está disponible.
 * Se llama desde la tarea de comunicación.
 *
 * @return true si se inició la transmisión, false si el buffer está vacío o ocupado.
 */
bool usart_hw_transmit_from_system_buffer(void *context);

/**
 * @brief Detener recepción DMA en USART2.
 *
 * Deshabilita la interrupción IDLE y detiene el DMA de recepción.
 */
void usart_hw_stop_rx(void *context);

#endif