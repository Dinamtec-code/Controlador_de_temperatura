#include "config_manager.h"
#include "communication/comm_sys_api.h"
#include "stm32f3xx_hal_flash.h"
#include "stm32f3xx_hal_flash_ex.h"
#include <string.h>

/* ========================================================================== */
/* ESTRUCTURA PRIVADA                                                        */
/* ========================================================================== */

typedef struct
{
    comm_iface_id_t preferred_iface;
    sensor_type_t preferred_sensor;
    pid_config_t default_pid;
} system_config_t;

static system_config_t sys_config;

/* ========================================================================== */
/* PERSISTENCIA EN FLASH                                                     */
/* ========================================================================== */

#define CONFIG_FLASH_ADDR ((uint32_t)0x0800F800) /* última página de 2 KB */
#define CONFIG_FLASH_PAGE ((CONFIG_FLASH_ADDR - FLASH_BASE) / FLASH_PAGE_SIZE)
#define CONFIG_MAGIC 0xABCD1234u

/* Estructura en Flash (cada campo es uint32_t, los floats se almacenan como uint32_t) */
typedef struct
{
    uint32_t magic;
    uint32_t preferred_iface;
    uint32_t preferred_sensor;
    uint32_t pid_kp;
    uint32_t pid_ki;
    uint32_t pid_kd;
} flash_config_t;

/* ========================================================================== */
/* VALORES POR DEFECTO                                                        */
/* ========================================================================== */

void config_load_defaults(void)
{
    sys_config.preferred_iface = COMM_IFACE_USART;
    sys_config.preferred_sensor = SENSOR_RTD;
    sys_config.default_pid.kp = 1.0f;
    sys_config.default_pid.ki = 0.1f;
    sys_config.default_pid.kd = 0.0f;
}

/* ========================================================================== */
/* PERSISTENCIA (implementación real)                                          */
/* ========================================================================== */

/**
 * @brief Carga la configuración desde la Flash.
 * @return true si los datos son válidos y se cargaron correctamente.
 */
bool config_load_from_flash(void)
{
    const flash_config_t *pFlash = (const flash_config_t *)CONFIG_FLASH_ADDR;

    // Verificar marca mágica
    if (pFlash->magic != CONFIG_MAGIC)
    {
        return false;
    }

    // Cargar los valores (respetando el tipo de cada campo)
    sys_config.preferred_iface = (comm_iface_id_t)pFlash->preferred_iface;
    sys_config.preferred_sensor = (sensor_type_t)pFlash->preferred_sensor;

    // Copiar floats preservando su representación binaria
    uint32_t tmp;
    tmp = pFlash->pid_kp;
    memcpy(&sys_config.default_pid.kp, &tmp, sizeof(float));
    tmp = pFlash->pid_ki;
    memcpy(&sys_config.default_pid.ki, &tmp, sizeof(float));
    tmp = pFlash->pid_kd;
    memcpy(&sys_config.default_pid.kd, &tmp, sizeof(float));

    return true;
}

/**
 * @brief Guarda la configuración actual en la Flash.
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

    // 3. Preparar los datos en una estructura auxiliar
    flash_config_t flash_data;
    flash_data.magic = CONFIG_MAGIC;
    flash_data.preferred_iface = (uint32_t)sys_config.preferred_iface;
    flash_data.preferred_sensor = (uint32_t)sys_config.preferred_sensor;

    uint32_t tmp;
    memcpy(&tmp, &sys_config.default_pid.kp, sizeof(float));
    flash_data.pid_kp = tmp;
    memcpy(&tmp, &sys_config.default_pid.ki, sizeof(float));
    flash_data.pid_ki = tmp;
    memcpy(&tmp, &sys_config.default_pid.kd, sizeof(float));
    flash_data.pid_kd = tmp;

    // 4. Programar palabra por palabra
    uint32_t *src = (uint32_t *)&flash_data;
    uint32_t address = CONFIG_FLASH_ADDR;
    for (size_t i = 0; i < sizeof(flash_config_t) / sizeof(uint32_t); i++)
    {
        status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, src[i]);
        if (status != HAL_OK)
        {
            HAL_FLASH_Lock();
            return false;
        }
        address += 4;
    }

    // 5. Bloquear la Flash
    HAL_FLASH_Lock();
    return true;
}

/* ========================================================================== */
/* GETTERS                                                                   */
/* ========================================================================== */

comm_iface_id_t config_get_preferred_iface(void)
{
    return sys_config.preferred_iface;
}

sensor_type_t config_get_preferred_sensor(void)
{
    return sys_config.preferred_sensor;
}

pid_config_t config_get_default_pid(void)
{
    return sys_config.default_pid;
}

/* ========================================================================== */
/* SETTERS (con validación, persistencia y notificación)                      */
/* ========================================================================== */

void config_set_preferred_iface(comm_iface_id_t id)
{
    if (id >= COMM_IFACE_MAX)
        return;

    sys_config.preferred_iface = id;
    config_save_to_flash();
    comm_sys_request_iface_change(id);
}

void config_set_preferred_sensor(sensor_type_t type)
{
    sys_config.preferred_sensor = type;
    config_save_to_flash();
}

void config_set_default_pid(pid_config_t pid)
{
    sys_config.default_pid = pid;
    config_save_to_flash();
}