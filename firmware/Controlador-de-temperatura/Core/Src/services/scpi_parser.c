#include "services/scpi_parser.h"
#include "services/error_handler.h"
#include "main.h"
#include "hardware/adc_hw.h"
#include "usart.h"
#include "hardware/usart_hw.h"
#include "control/pid_controller.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const scpi_input_interface_t *input_iface = NULL;
static const scpi_output_interface_t *output_iface = NULL;
static const scpi_interface_t *scpi_iface_p = NULL;
static scpi_interface_t scpi_iface;

#define RESPONSE_BUFFER_SIZE 256
static uint8_t response_buffer[RESPONSE_BUFFER_SIZE];
static size_t response_len = 0;

void set_kp_callback(float value, void *context)
{
    pid_set_kp(get_temp_pid_instance(), value);
}

void set_ki_callback(float value, void *context)
{
    pid_set_ki(get_temp_pid_instance(), value);
}

void set_kd_callback(float value, void *context)
{
    pid_set_kd(get_temp_pid_instance(), value);
}

float get_kp_callback(void *context)
{
    return pid_get_kp(get_temp_pid_instance());
}

float get_ki_callback(void *context)
{
    return pid_get_ki(get_temp_pid_instance());
}

float get_kd_callback(void *context)
{
    return pid_get_kd(get_temp_pid_instance());
}

void set_setpoint_callback(float value, void *context)
{
    pid_set_setpoint(get_temp_pid_instance(), value);
}

float get_setpoint_callback(void *context)
{
    return pid_get_setpoint(get_temp_pid_instance());
}

float get_temperature_callback(void *context)
{
    return adc_hw_get_temp_diff();
}

float get_out_callback(void *context)
{
    return pid_get_out(get_temp_pid_instance());
}

bool get_output_on_callback(int channel, void *context)
{
    return pid_controller_get_output(get_temp_pid_instance(), channel);
}

void set_output_on_callback(int channel, bool value, void *context)
{
    pid_controller_set_output(get_temp_pid_instance(), channel, value);
    if (channel == 0)
    {
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, value ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
    else if (channel == 1)
    {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_4, value ? GPIO_PIN_RESET : GPIO_PIN_SET);
    }
}

void scpi_init(void)
{
    scpi_iface.get_temp = get_temperature_callback;
    scpi_iface.get_sp = get_setpoint_callback;
    scpi_iface.set_sp = set_setpoint_callback;
    scpi_iface.set_kp = set_kp_callback;
    scpi_iface.set_ki = set_ki_callback;
    scpi_iface.set_kd = set_kd_callback;
    scpi_iface.get_kp = get_kp_callback;
    scpi_iface.get_ki = get_ki_callback;
    scpi_iface.get_kd = get_kd_callback;
    scpi_iface.get_out = get_out_callback;
    scpi_iface.get_output_on = get_output_on_callback;
    scpi_iface.set_output_on = set_output_on_callback;
    scpi_iface.context = NULL;
    scpi_iface_p = &scpi_iface;
    input_iface = NULL;
    output_iface = NULL;
    response_len = 0;
}

void scpi_set_input_interface(const scpi_input_interface_t *in_iface)
{
    input_iface = in_iface;
}

void scpi_set_output_interface(const scpi_output_interface_t *out_iface)
{
    output_iface = out_iface;
}

void scpi_flush_responses(void)
{
    if (response_len > 0)
    {
        response_buffer[response_len] = '\n';
        response_len++;
        response_buffer[response_len] = '\0';
        response_len++;
        if (output_iface && output_iface->send_response)
        {
            output_iface->send_response((const char *)response_buffer, (void *)output_iface->context);
        }
        response_len = 0;
    }
}

static void buffer_response(const char *resp)
{
    size_t resp_len = strlen(resp);

    if (response_len == 0)
    {
        for (size_t i = 0; i < resp_len && i < RESPONSE_BUFFER_SIZE - 2; i++)
        {
            response_buffer[response_len++] = resp[i];
        }
    }
    else
    {
        if (response_len + resp_len < RESPONSE_BUFFER_SIZE)
        {
            response_buffer[response_len] = ';';
            response_len++;
            for (size_t i = 0; i < resp_len && response_len < RESPONSE_BUFFER_SIZE - 2; i++)
            {
                response_buffer[response_len++] = resp[i];
            }
        }
    }
}

void scpi_process_message(const char *message)
{
    if (!scpi_iface_p || !message)
    {
        return;
    }

    response_len = 0;

    char cmd[128];
    size_t cmd_len = 0;

    for (size_t i = 0; i < strlen(message); i++)
    {
        if (message[i] == ';' || message[i] == '\n' || message[i] == '\0')
        {
            cmd[cmd_len] = '\0';
            if (cmd_len > 0)
            {
                scpi_process_line(cmd);
            }
            cmd_len = 0;
        }
        else
        {
            if (cmd_len < sizeof(cmd) - 1)
            {
                cmd[cmd_len++] = message[i];
            }
            else
            {
                error_set(ERROR_RX_COMMAND_OVERFLOW);
            }
        }
    }

    if (cmd_len > 0)
    {
        cmd[cmd_len] = '\0';
        scpi_process_line(cmd);
    }

    scpi_flush_responses();
}

void scpi_process_line(const char *command)
{
    if (!scpi_iface_p)
    {
        return;
    }

    if (strncmp(command, "*IDN?", 5) == 0)
    {
        buffer_response("TEMP-CTRL-CT2CH-V1.01");
    }
    else if (strncmp(command, "*CLS", 4) == 0)
    {
    }
    else if (strncmp(command, "*RST", 4) == 0)
    {
        if (scpi_iface_p->set_sp)
        {
            scpi_iface_p->set_sp(25.0f, scpi_iface_p->context);
        }
        if (scpi_iface_p->set_kp)
        {
            scpi_iface_p->set_kp(0.0f, scpi_iface_p->context);
        }
        if (scpi_iface_p->set_ki)
        {
            scpi_iface_p->set_ki(0.0f, scpi_iface_p->context);
        }
        if (scpi_iface_p->set_kd)
        {
            scpi_iface_p->set_kd(0.0f, scpi_iface_p->context);
        }
    }
    else if (strncmp(command, "MEAS:TEMP?", 10) == 0)
    {
        float temp = scpi_iface_p->get_temp(scpi_iface_p->context);
        char resp[16];

        int int_part = (int)temp;
        int dec_part = (int)((temp - (float)int_part) * 100.0f);

        if (dec_part < 0)
            dec_part = -dec_part;

        snprintf(resp, sizeof(resp), "%d.%02d", int_part, dec_part);
        buffer_response(resp);
    }
    else if (strncmp(command, "TEMP:SP?", 8) == 0)
    {
        float sp = scpi_iface_p->get_sp(scpi_iface_p->context);
        char resp[16];

        int int_part = (int)sp;
        int dec_part = (int)((sp - (float)int_part) * 100.0f);
        if (dec_part < 0)
            dec_part = -dec_part;

        snprintf(resp, sizeof(resp), "%d.%02d", int_part, dec_part);
        buffer_response(resp);
    }
    else if (strncmp(command, "TEMP:SP ", 8) == 0)
    {
        float val = atof(command + 8);
        if (val <= -10)
        {
            val = -10;
        }
        else if (val >= 150)
        {
            val = 120;
        }
        scpi_iface_p->set_sp(val, scpi_iface_p->context);
    }
    else if (strncmp(command, "PID:KP?", 7) == 0)
    {
        float kp = scpi_iface_p->get_kp(scpi_iface_p->context);
        char resp[20];

        int int_part = (int)kp;
        int dec_part = (int)((kp - (float)int_part) * 10000.0f);
        if (dec_part < 0)
            dec_part = -dec_part;

        snprintf(resp, sizeof(resp), "%d.%04d", int_part, dec_part);
        buffer_response(resp);
    }
    else if (strncmp(command, "PID:KI?", 7) == 0)
    {
        float ki = scpi_iface_p->get_ki(scpi_iface_p->context);
        char resp[20];

        int int_part = (int)ki;
        int dec_part = (int)((ki - (float)int_part) * 10000.0f);
        if (dec_part < 0)
            dec_part = -dec_part;

        snprintf(resp, sizeof(resp), "%d.%04d", int_part, dec_part);
        buffer_response(resp);
    }
    else if (strncmp(command, "PID:KD?", 7) == 0)
    {
        float kd = scpi_iface_p->get_kd(scpi_iface_p->context);
        char resp[20];

        int int_part = (int)kd;
        int dec_part = (int)((kd - (float)int_part) * 10000.0f);
        if (dec_part < 0)
            dec_part = -dec_part;

        snprintf(resp, sizeof(resp), "%d.%04d", int_part, dec_part);
        buffer_response(resp);
    }
    else if (strncmp(command, "PID:DUTY?", 8) == 0)
    {
        float out = scpi_iface_p->get_out(scpi_iface_p->context);
        char resp[20];

        int int_part = (int)out;
        int dec_part = (int)((out - (float)int_part) * 10000.0f);
        if (dec_part < 0)
            dec_part = -dec_part;

        snprintf(resp, sizeof(resp), "%d.%04d", int_part, dec_part);
        buffer_response(resp);
    }
    else if (strncmp(command, "PID:KP ", 7) == 0)
    {
        scpi_iface_p->set_kp(atof(command + 7), scpi_iface_p->context);
    }
    else if (strncmp(command, "PID:KI ", 7) == 0)
    {
        scpi_iface_p->set_ki(atof(command + 7), scpi_iface_p->context);
    }
    else if (strncmp(command, "PID:KD ", 7) == 0)
    {
        scpi_iface_p->set_kd(atof(command + 7), scpi_iface_p->context);
    }
    else if (strncmp(command, "SOUR1:OUTP OFF", 14) == 0)
    {
        scpi_iface_p->set_output_on(0, false, scpi_iface_p->context);
    }
    else if (strncmp(command, "SOUR1:OUTP ON", 13) == 0)
    {
        scpi_iface_p->set_output_on(0, true, scpi_iface_p->context);
    }
    else if (strncmp(command, "SOUR2:OUTP OFF", 14) == 0)
    {
        scpi_iface_p->set_output_on(1, false, scpi_iface_p->context);
    }
    else if (strncmp(command, "SOUR2:OUTP ON", 13) == 0)
    {
        scpi_iface_p->set_output_on(1, true, scpi_iface_p->context);
    }
    else if (strncmp(command, "SOUR1:OUTP?", 11) == 0)
    {
        if (scpi_iface_p->get_output_on(0, scpi_iface_p->context))
        {
            buffer_response("ON");
        }
        else
        {
            buffer_response("OFF");
        }
    }
    else if (strncmp(command, "SOUR2:OUTP?", 11) == 0)
    {
        if (scpi_iface_p->get_output_on(1, scpi_iface_p->context))
        {
            buffer_response("ON");
        }
        else
        {
            buffer_response("OFF");
        }
    }
}
