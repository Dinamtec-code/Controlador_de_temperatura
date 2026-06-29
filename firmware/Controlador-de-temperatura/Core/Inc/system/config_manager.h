#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "communication/comm_driver_api.h"
#include <stdbool.h>

typedef enum
{
    SENSOR_RTD = 0,
    SENSOR_THERMOCOUPLE = 1,
    SENSOR_LM35 = 2
} sensor_type_t;

typedef struct
{
    float kp;
    float ki;
    float kd;
} pid_config_t;

/* -------------------- API de acceso -------------------- */

comm_iface_id_t config_get_preferred_iface(void);
void config_set_preferred_iface(comm_iface_id_t id);

sensor_type_t config_get_preferred_sensor(void);
void config_set_preferred_sensor(sensor_type_t type);

pid_config_t config_get_default_pid(void);
void config_set_default_pid(pid_config_t pid);

/* -------------------- Persistencia -------------------- */

void config_load_defaults(void);
bool config_load_from_flash(void);
bool config_save_to_flash(void);

#endif /* CONFIG_MANAGER_H */