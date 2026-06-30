#include "scpi/scpi_engine.h"
#include "scpi/scpi_msg_api.h"
#include "scpi/scpi.h"

#include "communication/app_msg_api.h"
#include "tasks/task_comm.h"
#include "system/config_manager.h"
#include "control/pid_controller.h"
#include "hardware/adc_hw.h"
#include "main.h"
#include <string.h>
#include <stdio.h>

/* ==========================================================================
 * Contexto global de libscpi y buffers asociados
 * ========================================================================== */

static scpi_t scpi_context;

#define SCPI_INPUT_BUFFER_LENGTH 256
static char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];

#define SCPI_ERROR_QUEUE_SIZE 10
static scpi_error_t scpi_error_queue[SCPI_ERROR_QUEUE_SIZE];

static scpi_result_t my_CoreRst(scpi_t *context);

/* ==========================================================================
 * Funciones auxiliares de los comandos del instrumento
 * ========================================================================== */

static float get_temperature(void)
{
    return adc_hw_get_temp_diff();
}

static float get_setpoint(void)
{
    return pid_get_setpoint(get_pid_instance((uint8_t)0));
}

static void set_setpoint(float val)
{
    pid_set_setpoint(get_pid_instance(0), val);
}

static float get_kp(void)
{
    return pid_get_kp(get_pid_instance(0));
}

static void set_kp(float val)
{
    pid_set_kp(get_pid_instance(0), val);
}

static float get_ki(void)
{
    return pid_get_ki(get_pid_instance(0));
}

static void set_ki(float val)
{
    pid_set_ki(get_pid_instance(0), val);
}

static float get_kd(void)
{
    return pid_get_kd(get_pid_instance(0));
}

static void set_kd(float val)
{
    pid_set_kd(get_pid_instance(0), val);
}

static float get_duty(void)
{
    return pid_get_out(get_pid_instance(0));
}

static bool get_output(uint8_t channel)
{
    return pid_controller_get_output(get_pid_instance(0), channel);
}

static void set_output(uint8_t channel, bool on)
{
    pid_controller_set_output(get_pid_instance(0), channel, on);
    if (channel == 0)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
    else if (channel == 1)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, on ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

/* ==========================================================================
 * Callbacks de comandos SCPI
 * ========================================================================== */

static scpi_result_t cmd_meas_temp(scpi_t *context)
{
    float temp = get_temperature();
    SCPI_ResultFloat(context, temp);
    HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
    return SCPI_RES_OK;
}

static scpi_result_t cmd_temp_sp_q(scpi_t *context)
{
    float sp = get_setpoint();
    SCPI_ResultFloat(context, sp);
    return SCPI_RES_OK;
}

static scpi_result_t cmd_temp_sp(scpi_t *context)
{
    float val;
    if (SCPI_ParamFloat(context, &val, true))
    {
        if (val < -10.0f)
            val = -10.0f;
        if (val > 150.0f)
            val = 120.0f;
        set_setpoint(val);
        return SCPI_RES_OK;
    }
    return SCPI_RES_ERR;
}

static scpi_result_t cmd_pid_kp_q(scpi_t *context)
{
    SCPI_ResultFloat(context, get_kp());
    return SCPI_RES_OK;
}

static scpi_result_t cmd_pid_kp(scpi_t *context)
{
    float val;
    if (SCPI_ParamFloat(context, &val, true))
    {
        set_kp(val);
        return SCPI_RES_OK;
    }
    return SCPI_RES_ERR;
}

static scpi_result_t cmd_pid_ki_q(scpi_t *context)
{
    SCPI_ResultFloat(context, get_ki());
    return SCPI_RES_OK;
}

static scpi_result_t cmd_pid_ki(scpi_t *context)
{
    float val;
    if (SCPI_ParamFloat(context, &val, true))
    {
        set_ki(val);
        return SCPI_RES_OK;
    }
    return SCPI_RES_ERR;
}

static scpi_result_t cmd_pid_kd_q(scpi_t *context)
{
    SCPI_ResultFloat(context, get_kd());
    return SCPI_RES_OK;
}

static scpi_result_t cmd_pid_kd(scpi_t *context)
{
    float val;
    if (SCPI_ParamFloat(context, &val, true))
    {
        set_kd(val);
        return SCPI_RES_OK;
    }
    return SCPI_RES_ERR;
}

static scpi_result_t cmd_pid_duty_q(scpi_t *context)
{
    SCPI_ResultFloat(context, get_duty());
    return SCPI_RES_OK;
}

static scpi_result_t cmd_sour_outp(scpi_t *context)
{
    uint64_t channel;
    scpi_bool_t on;
    if (!SCPI_ParamUInt64(context, &channel, true))
        return SCPI_RES_ERR;
    if (channel > 1)
        return SCPI_RES_ERR;
    if (!SCPI_ParamBool(context, &on, true))
        return SCPI_RES_ERR;
    set_output((uint8_t)channel, on);
    return SCPI_RES_OK;
}

static scpi_result_t cmd_sour_outp_q(scpi_t *context)
{
    uint64_t channel;
    if (!SCPI_ParamUInt64(context, &channel, true))
        return SCPI_RES_ERR;
    if (channel > 1)
        return SCPI_RES_ERR;
    SCPI_ResultBool(context, get_output((uint8_t)channel));
    return SCPI_RES_OK;
}

/**
 * Reimplement IEEE488.2 *TST?
 *
 * Result should be 0 if everything is ok
 * Result should be 1 if something goes wrong
 *
 * Return SCPI_RES_OK
 */
static scpi_result_t My_CoreTstQ(scpi_t *context)
{

    SCPI_ResultInt32(context, 0);

    return SCPI_RES_OK;
}

/* ==========================================================================
 * Árbol de comandos del instrumento
 * ========================================================================== */

static const scpi_command_t scpi_commands[] = {
    /* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
    {
        .pattern = "*CLS",
        .callback = SCPI_CoreCls,
    },
    {
        .pattern = "*ESE",
        .callback = SCPI_CoreEse,
    },
    {
        .pattern = "*ESE?",
        .callback = SCPI_CoreEseQ,
    },
    {
        .pattern = "*ESR?",
        .callback = SCPI_CoreEsrQ,
    },
    {
        .pattern = "*IDN?",
        .callback = SCPI_CoreIdnQ,
    },
    {
        .pattern = "*OPC",
        .callback = SCPI_CoreOpc,
    },
    {
        .pattern = "*OPC?",
        .callback = SCPI_CoreOpcQ,
    },
    {
        .pattern = "*RST",
        .callback = my_CoreRst,
        //        .callback = SCPI_CoreRst,
    },
    {
        .pattern = "*SRE",
        .callback = SCPI_CoreSre,
    },
    {
        .pattern = "*SRE?",
        .callback = SCPI_CoreSreQ,
    },
    {
        .pattern = "*STB?",
        .callback = SCPI_CoreStbQ,
    },
    {
        .pattern = "*TST?",
        .callback = My_CoreTstQ,
    },
    {
        .pattern = "*WAI",
        .callback = SCPI_CoreWai,
    },

    /* Required SCPI commands (SCPI std V1999.0 4.2.1) */
    {
        .pattern = "SYSTem:ERRor[:NEXT]?",
        .callback = SCPI_SystemErrorNextQ,
    },
    {
        .pattern = "SYSTem:ERRor:COUNt?",
        .callback = SCPI_SystemErrorCountQ,
    },
    {
        .pattern = "SYSTem:VERSion?",
        .callback = SCPI_SystemVersionQ,
    },
    /* {.pattern = "STATus:OPERation?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:EVENt?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:CONDition?", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle", .callback = scpi_stub_callback,}, */
    /* {.pattern = "STATus:OPERation:ENABle?", .callback = scpi_stub_callback,}, */
    {
        .pattern = "STATus:QUEStionable[:EVENt]?",
        .callback = SCPI_StatusQuestionableEventQ,
    },
    /* {.pattern = "STATus:QUEStionable:CONDition?", .callback = scpi_stub_callback,}, */
    {
        .pattern = "STATus:QUEStionable:ENABle",
        .callback = SCPI_StatusQuestionableEnable,
    },
    {
        .pattern = "STATus:QUEStionable:ENABle?",
        .callback = SCPI_StatusQuestionableEnableQ,
    },

    {
        .pattern = "STATus:PRESet",
        .callback = SCPI_StatusPreset,
    },
    /* Medición */
    {.pattern = "MEASure:TEMPerature?", .callback = cmd_meas_temp, .tag = 0},

    /* Setpoint */
    {.pattern = "SOURce:TEMPerature:SPOint?", .callback = cmd_temp_sp_q, .tag = 0},
    {.pattern = "SOURce:TEMPerature:SPOint", .callback = cmd_temp_sp, .tag = 0},

    /* PID */
    {.pattern = "SOURce:CONTrol:PID:KP?", .callback = cmd_pid_kp_q, .tag = 0},
    {.pattern = "SOURce:CONTrol:PID:KP", .callback = cmd_pid_kp, .tag = 0},
    {.pattern = "SOURce:CONTrol:PID:KI?", .callback = cmd_pid_ki_q, .tag = 0},
    {.pattern = "SOURce:CONTrol:PID:KI", .callback = cmd_pid_ki, .tag = 0},
    {.pattern = "SOURce:CONTrol:PID:KD?", .callback = cmd_pid_kd_q, .tag = 0},
    {.pattern = "SOURce:CONTrol:PID:KD", .callback = cmd_pid_kd, .tag = 0},
    {.pattern = "SOURce:CONTrol:PID:OUTPut?", .callback = cmd_pid_duty_q, .tag = 0},

    /* Salidas */
    {.pattern = "SOURce:OUTPut", .callback = cmd_sour_outp, .tag = 0},
    {.pattern = "SOURce:OUTPut?", .callback = cmd_sour_outp_q, .tag = 0},

    SCPI_CMD_LIST_END};

/* ==========================================================================
 * Callbacks de la interfaz de libscpi
 * ========================================================================== */

static scpi_result_t my_CoreRst(scpi_t *context)
{
    (void)context;
    SCPI_CoreRst(context);       // Reset del parser y registros IEEE 488.2
    config_apply_reset_values(); // Restauración de los parámetros del instrumento
    return SCPI_RES_OK;
}

static size_t scpi_write_cb(scpi_t *ctx, const char *data, size_t len)
{
    (void)ctx;
    comm_app_send_response((const uint8_t *)data, len);
    return len;
}

static int scpi_error_cb(scpi_t *ctx, int_fast16_t err)
{
    (void)ctx;
    (void)err;
    return SCPI_RES_OK;
}

static scpi_result_t scpi_reset_cb(scpi_t *ctx)
{
    (void)ctx;
    return SCPI_RES_OK;
}

static scpi_result_t scpi_control_cb(scpi_t *ctx, scpi_ctrl_name_t ctrl, scpi_reg_val_t val)
{
    (void)ctx;
    (void)val;
    (void)ctrl;
    return SCPI_RES_OK;
}

static scpi_result_t scpi_flush_cb(scpi_t *ctx)
{
    (void)ctx;
    return SCPI_RES_OK;
}

static scpi_interface_t scpi_interface = {
    .error = scpi_error_cb,
    .write = scpi_write_cb,
    .control = scpi_control_cb,
    .flush = scpi_flush_cb,
    .reset = scpi_reset_cb,
};

/* ==========================================================================
 * Implementación de app_msg_iface_t
 * ========================================================================== */

static void on_rx_data(void *ctx, const uint8_t *data, size_t len)
{
    (void)ctx;
    SCPI_Input(&scpi_context, (const char *)data, len);
}

static app_msg_response_t on_tx_done(void *ctx)
{
    (void)ctx;
    return APP_MSG_OK;
}

static void on_error(void *ctx, app_msg_error_t error)
{
    (void)ctx;
    (void)error;
}

static app_msg_iface_t app_iface = {
    .context = NULL,
    .on_rx_data = on_rx_data,
    .on_tx_done = on_tx_done,
    .on_error = on_error,
};

/* ==========================================================================
 * API pública del parser SCPI
 * ========================================================================== */

void scpi_engine_init(void)
{
    SCPI_Init(&scpi_context,
              scpi_commands,
              &scpi_interface,
              scpi_units_def,
              "DF",
              "CONTROL DE TEMPERATURA",
              "CT2CH",
              "v1.1",
              scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
              scpi_error_queue, SCPI_ERROR_QUEUE_SIZE);
}

app_msg_iface_t *scpi_engine_get_iface(void)
{
    return &app_iface;
}