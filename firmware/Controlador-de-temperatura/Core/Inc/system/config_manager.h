#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "communication/comm_driver_api.h"
#include <stdbool.h>
#include <stdint.h>

/* ========================================================================== */
/* TIPOS PÚBLICOS                                                             */
/* ========================================================================== */

typedef enum
{
    SENSOR_RTD = 0,
    SENSOR_THERMOCOUPLE = 1
} sensor_type_t;

typedef struct
{
    float kp;
    float ki;
    float kd;
    float setpoint;
    float out_min;
    float out_max;
} pid_config_t;

/* ========================================================================== */
/* CARGA Y PERSISTENCIA                                                       */
/* ========================================================================== */

void config_load_defaults(void);
bool config_load_from_flash(void);
bool config_save_to_flash(void);

/* ========================================================================== */
/* RESET DEL INSTRUMENTO                                                      */
/* ========================================================================== */

void config_apply_reset_values(void);

/* ========================================================================== */
/* GETTERS (sobre el slot activo)                                             */
/* ========================================================================== */

comm_iface_id_t config_get_preferred_iface(void);
sensor_type_t config_get_preferred_sensor(uint8_t channel);
pid_config_t config_get_pid(uint8_t channel);

/* ========================================================================== */
/* SETTERS (con validación, persistencia y notificación)                      */
/* ========================================================================== */

void config_set_preferred_iface(comm_iface_id_t id);
void config_set_preferred_sensor(uint8_t channel, sensor_type_t type);
void config_set_pid(uint8_t channel, pid_config_t pid);

/* ========================================================================== */
/* GESTIÓN DE SLOTS                                                           */
/* ========================================================================== */

uint8_t config_get_active_slot(void);
bool config_set_active_slot(uint8_t slot);
bool config_save_to_slot(uint8_t slot, const char *name);
bool config_load_from_slot(uint8_t slot);
const char *config_get_slot_name(uint8_t slot);

#endif /* CONFIG_MANAGER_H */