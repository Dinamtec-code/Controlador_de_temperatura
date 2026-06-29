#include "tasks/task_system.h"
#include "system/config_manager.h"

#include "communication/comm_sys_api.h"

#include "services/error_handler.h"

#include "hardware/usart_hw.h"
#include "hardware/adc_hw.h"
#include "hardware/oled_hw.h"

#include "main.h"

static bool is_config_load = 0;
static bool is_comm_init = 0;
static bool is_i2c_init = 0;
static bool is_pwm_init = 0;
static bool is_adc_init = 0;
static bool is_panel_init = 0;
static bool is_display_init = 0;

inline static void system_runtime(void)
{
    // TODO: por defecto el sistema revisa el estado de sus componentes y los inicia/reinicia si funcionan mal
    // Revisar estado de comunicación e iniciar, reinicia ó detiene si el sistema lo necesita.
    // Revisar estado de pantalla e iniciar ó reiniciar si algo anda mal.
    //
}

void task_system(void)
{
    // 1. Cargar configuración (solo una vez)
    if (!is_config_load)
    {
        config_load_defaults();
        if (!config_load_from_flash())
        {
            // Se mantienen los valores por defecto; el sistema arranca igual.
            // Opcional: registrar un evento de advertencia.
        }
        is_config_load = 1;
    }

    // 2. Inicializaciones que dependen de la configuración
    if (!is_comm_init && is_config_load)
    {
        cb_t *rx, *tx;
        comm_task_get_buffers(&rx, &tx);
        if (usart_iface_register(rx, tx) == COMM_IFACE_OK)
        {
            is_comm_init = 1;
            comm_sys_start();
        }
    }

    if (!is_pwm_init)
    {
        /*Registrar perifericos control de potencia*/
        MX_HRTIM1_Init();
        is_pwm_init = 1;
    }

    if (!is_adc_init)
    {
        /*Registrar perifericos de medición*/
        MX_ADC1_Init();
        MX_ADC2_Init();
        is_adc_init = 1;
    }

    if (!is_panel_init)
    {
        /*Registrar perifericos de interacción (panel frontal)*/
        //...
    }

    if (!is_i2c_init)
    {
        i2c_hw_init();
        is_i2c_init = 1;
    }

    if (is_i2c_init)
    {
        if (!is_display_init)
        {
            oled_hw_init();
            is_display_init = 1;
        }
        // OTROS perifericos dependientes de i2c
        // ...
    }
    system_runtime();
}