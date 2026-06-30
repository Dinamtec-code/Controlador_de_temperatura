#ifndef SCPI_ENGINE_H
#define SCPI_ENGINE_H

#include "communication/app_msg_api.h"

/**
 * @brief Inicializa el motor SCPI (libscpi) y configura los comandos
 *        específicos del instrumento.
 */
void scpi_engine_init(void);

/**
 * @brief Obtiene la interfaz de aplicación que la Communication Task
 *        utilizará para entregar mensajes y notificaciones.
 * @return Puntero a la estructura app_msg_iface_t implementada por el motor.
 */
app_msg_iface_t *scpi_engine_get_iface(void);

#endif /* SCPI_ENGINE_H */