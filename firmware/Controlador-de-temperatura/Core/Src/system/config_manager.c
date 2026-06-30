#include "system/config_manager.h"
#include "communication/comm_sys_api.h"
#include "control/pid_controller.h"
#include "main.h"
#include <string.h>

/* ========================================================================== */
/* ESTRUCTURA PRIVADA                                                        */
/* ========================================================================== */

#define NUM_CHANNELS PID_CHANNELS
#define MAX_CONFIGS 4     // [0] = default, [1..3] = personalizadas
#define CONFIG_NAME_LEN 8 // "PELT\0", "RESI\0", etc.

typedef struct
{
    float kp;
    float ki;
    float kd;
    float setpoint;
    float out_min;
    float out_max;
} channel_config_t;

typedef struct
{
    char name[CONFIG_NAME_LEN];
    channel_config_t channels[NUM_CHANNELS];
    sensor_type_t sensors[NUM_CHANNELS];
    comm_iface_id_t preferred_iface;
} config_slot_t;

typedef struct
{
    config_slot_t slots[MAX_CONFIGS];
    uint8_t active_slot; // índice de la configuración activa
} system_config_t;

static system_config_t sys_config;

/* ========================================================================== */
/* VALORES POR DEFECTO                                                        */
/* ========================================================================== */

void config_load_defaults(void)
{
    memset(&sys_config, 0, sizeof(sys_config));
    sys_config.active_slot = 0;
    strncpy(sys_config.slots[0].name, "DEFAULT", CONFIG_NAME_LEN - 1);
    sys_config.slots[0].preferred_iface = COMM_IFACE_USART;

    for (int ch = 0; ch < NUM_CHANNELS; ch++)
    {
        sys_config.slots[0].channels[ch].kp = 1.0f;
        sys_config.slots[0].channels[ch].ki = 0.1f;
        sys_config.slots[0].channels[ch].kd = 0.0f;
        sys_config.slots[0].channels[ch].setpoint = 25.0f;
        sys_config.slots[0].channels[ch].out_min = 0.0f;
        sys_config.slots[0].channels[ch].out_max = 50.0f;
        sys_config.slots[0].sensors[ch] = SENSOR_RTD;
    }

    // Marcar slots 1..3 como vacíos
    for (int s = 1; s < MAX_CONFIGS; s++)
    {
        sys_config.slots[s].name[0] = '\0';
    }
}

/* ========================================================================== */
/* PERSISTENCIA EN FLASH                                                     */
/* ========================================================================== */

#define CONFIG_FLASH_ADDR ((uint32_t)0x0800F800) /* última página de 2 KB */
#define CONFIG_FLASH_PAGE ((CONFIG_FLASH_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE)
#define CONFIG_MAGIC 0xABCD1234u

/**
 * @brief Carga la configuración completa desde la Flash.
 * @return true si los datos son válidos y se cargaron correctamente.
 */
bool config_load_from_flash(void)
{
    const system_config_t *pFlash = (const system_config_t *)CONFIG_FLASH_ADDR;

    // Verificar marca mágica (ubicada al inicio de la estructura)
    if (*(uint32_t *)pFlash != CONFIG_MAGIC)
    {
        return false;
    }

    // Copiar toda la estructura desde la flash a la RAM
    memcpy(&sys_config, pFlash, sizeof(sys_config));

    // Verificar integridad básica de los campos
    if (sys_config.active_slot >= MAX_CONFIGS)
    {
        sys_config.active_slot = 0;
    }

    return true;
}

/**
 * @brief Guarda la configuración completa en la Flash.
 *        Borra la página y programa los datos.
 * @return true si la operación fue exitosa.
 */
bool config_save_to_flash(void)
{
    HAL_StatusTypeDef status;
    uint32_t page_error = 0;
    FLASH_EraseInitTypeDef erase_init = {
        .TypeErase = FLASH_TYPEERASE_PAGES,
        .PageAddress = CONFIG_FLASH_ADDR,
        .NbPages = 1};

    // 1. Desbloquear la Flash
    HAL_FLASH_Unlock();

    // 2. Borrar la página
    status = HAL_FLASHEx_Erase(&erase_init, &page_error);
    if (status != HAL_OK)
    {
        HAL_FLASH_Lock();
        return false;
    }

    // 3. Programar la estructura completa palabra por palabra
    uint32_t *src = (uint32_t *)&sys_config;
    uint32_t address = CONFIG_FLASH_ADDR;
    for (size_t i = 0; i < sizeof(sys_config) / sizeof(uint32_t); i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, src[i]);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
        address += 4;
    }

    // 4. Bloquear la Flash
    HAL_FLASH_Lock();
    return true;
}

/******************************************************************************
 * Configuración de reset (aplica valores del slot activo)
 ******************************************************************************/
void config_apply_reset_values(void)
{
    const config_slot_t *slot = &sys_config.slots[sys_config.active_slot];

    for (uint8_t ch = 0; ch < NUM_CHANNELS; ch++)
    {
        pid_controller_t *pid = get_pid_instance(ch);
        if (pid)
        {
            pid_set_setpoint(pid, slot->channels[ch].setpoint);
            pid_set_kp(pid, slot->channels[ch].kp);
            pid_set_ki(pid, slot->channels[ch].ki);
            pid_set_kd(pid, slot->channels[ch].kd);
            pid_controller_set_output(pid, ch, false); // apagar salida
        }

        // El tipo de sensor se aplicará cuando exista la función correspondiente
        // config_apply_sensor_type(ch, slot->sensors[ch]);
    }
}

/* ========================================================================== */
/* GETTERS (sobre el slot activo)                                             */
/* ========================================================================== */

comm_iface_id_t config_get_preferred_iface(void)
{
    return sys_config.slots[sys_config.active_slot].preferred_iface;
}

sensor_type_t config_get_preferred_sensor(uint8_t channel)
{
    if (channel >= NUM_CHANNELS)
        return SENSOR_RTD;
    return sys_config.slots[sys_config.active_slot].sensors[channel];
}

pid_config_t config_get_pid(uint8_t channel)
{
    pid_config_t pid = {0};
    if (channel < NUM_CHANNELS)
    {
        const channel_config_t *ch = &sys_config.slots[sys_config.active_slot].channels[channel];
        pid.kp = ch->kp;
        pid.ki = ch->ki;
        pid.kd = ch->kd;
        pid.setpoint = ch->setpoint;
        pid.out_min = ch->out_min;
        pid.out_max = ch->out_max;
    }
    return pid;
}

/* ========================================================================== */
/* SETTERS (con validación, persistencia y notificación)                      */
/* ========================================================================== */

void config_set_preferred_iface(comm_iface_id_t id)
{
    if (id >= COMM_IFACE_MAX)
        return;

    sys_config.slots[sys_config.active_slot].preferred_iface = id;
    config_save_to_flash();
    comm_sys_request_iface_change(id);
}

void config_set_preferred_sensor(uint8_t channel, sensor_type_t type)
{
    if (channel >= NUM_CHANNELS)
        return;

    sys_config.slots[sys_config.active_slot].sensors[channel] = type;
    config_save_to_flash();
}

void config_set_pid(uint8_t channel, pid_config_t pid)
{
    if (channel >= NUM_CHANNELS)
        return;

    sys_config.slots[sys_config.active_slot].channels[channel].kp = pid.kp;
    sys_config.slots[sys_config.active_slot].channels[channel].ki = pid.ki;
    sys_config.slots[sys_config.active_slot].channels[channel].kd = pid.kd;
    sys_config.slots[sys_config.active_slot].channels[channel].setpoint = pid.setpoint;
    sys_config.slots[sys_config.active_slot].channels[channel].out_min = pid.out_min;
    sys_config.slots[sys_config.active_slot].channels[channel].out_max = pid.out_max;
    config_save_to_flash();
}

/* ========================================================================== */
/* GESTIÓN DE SLOTS                                                           */
/* ========================================================================== */

/**
 * @brief Cambia el slot de configuración activo.
 * @param slot Índice del slot (0..MAX_CONFIGS-1).
 * @return true si el cambio fue exitoso.
 */
bool config_set_active_slot(uint8_t slot)
{
    if (slot >= MAX_CONFIGS)
        return false;
    if (sys_config.slots[slot].name[0] == '\0')
        return false; // slot vacío

    sys_config.active_slot = slot;
    return true;
}

/**
 * @brief Obtiene el índice del slot activo.
 */
uint8_t config_get_active_slot(void)
{
    return sys_config.active_slot;
}

/**
 * @brief Guarda la configuración actual en un slot con nombre.
 * @param slot Índice del slot donde guardar.
 * @param name Nombre identificativo (máx. CONFIG_NAME_LEN-1 caracteres).
 * @return true si se guardó correctamente.
 */
bool config_save_to_slot(uint8_t slot, const char *name)
{
    if (slot >= MAX_CONFIGS || slot == 0)
        return false; // slot 0 reservado para default
    if (name == NULL || name[0] == '\0')
        return false;

    strncpy(sys_config.slots[slot].name, name, CONFIG_NAME_LEN - 1);
    sys_config.slots[slot].name[CONFIG_NAME_LEN - 1] = '\0';

    // Copiar los datos del slot activo al slot destino
    memcpy(&sys_config.slots[slot].channels,
           &sys_config.slots[sys_config.active_slot].channels,
           sizeof(sys_config.slots[0].channels));
    memcpy(&sys_config.slots[slot].sensors,
           &sys_config.slots[sys_config.active_slot].sensors,
           sizeof(sys_config.slots[0].sensors));
    sys_config.slots[slot].preferred_iface = sys_config.slots[sys_config.active_slot].preferred_iface;

    return config_save_to_flash();
}

/**
 * @brief Carga la configuración desde un slot y la activa.
 * @param slot Índice del slot a cargar.
 * @return true si se cargó correctamente.
 */
bool config_load_from_slot(uint8_t slot)
{
    if (!config_set_active_slot(slot))
        return false;

    config_apply_reset_values();
    return true;
}

/**
 * @brief Obtiene el nombre de un slot (puntero al nombre, no copia).
 * @return NULL si el slot está vacío o es inválido.
 */
const char *config_get_slot_name(uint8_t slot)
{
    if (slot >= MAX_CONFIGS)
        return NULL;
    if (sys_config.slots[slot].name[0] == '\0')
        return NULL;
    return sys_config.slots[slot].name;
}