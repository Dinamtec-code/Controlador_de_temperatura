#include "services/scpi_parser.h"
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
    return adc_hw_read_temperature();
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
    scpi_iface.context = NULL;
    scpi_iface_p = &scpi_iface;
    input_iface = NULL;
    output_iface = NULL;
}

void scpi_set_input_interface(const scpi_input_interface_t *in_iface)
{
    input_iface = in_iface;
}

void scpi_set_output_interface(const scpi_output_interface_t *out_iface)
{
    output_iface = out_iface;
}

static void send_response(const char *resp)
{
    if (output_iface && output_iface->send_response)
    {
        output_iface->send_response(resp, (void *)output_iface->context);
    }
    else
    {
        usart_hw_send_str(resp);
    }
}

void scpi_process_line(const char *command)
{
    if (!scpi_iface_p)
    {
        return;
    }

    if (strncmp(command, "*IDN?", 5) == 0)
    {
        oled_hw_print_str_at("TEMPCTRL,STM32F334,1.0", 4, 0);
        send_response("TEMPCTRL,STM32F334,1.0\r\n");
        oled_hw_print_str_at("Response send", 6, 0);
        oled_hw_update();
    }
    else if (strncmp(command, "*CLS", 4) == 0)
    {
        send_response("OK\r\n");
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
        send_response("OK\r\n");
    }
    else if (strncmp(command, "MEAS:TEMP?", 10) == 0)
    {
        float temp = scpi_iface_p->get_temp(scpi_iface_p->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.2f\r\n", temp);
        send_response(resp);
    }
    else if (strncmp(command, "TEMP:SP?", 8) == 0)
    {
        float sp = scpi_iface_p->get_sp(scpi_iface_p->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.2f\r\n", sp);
        send_response(resp);
    }
    else if (strncmp(command, "TEMP:SP ", 8) == 0)
    {
        float val = atof(command + 8);
        if (val >= 0 && val <= 200)
        {
            scpi_iface_p->set_sp(val, scpi_iface_p->context);
            send_response("OK\r\n");
        }
        else
        {
            send_response("ERR\r\n");
        }
    }
    else if (strncmp(command, "PID:KP?", 7) == 0)
    {
        float kp = scpi_iface_p->get_kp(scpi_iface_p->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", kp);
        send_response(resp);
    }
    else if (strncmp(command, "PID:KI?", 7) == 0)
    {
        float ki = scpi_iface_p->get_ki(scpi_iface_p->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", ki);
        send_response(resp);
    }
    else if (strncmp(command, "PID:KD?", 7) == 0)
    {
        float kd = scpi_iface_p->get_kd(scpi_iface_p->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", kd);
        send_response(resp);
    }
    else if (strncmp(command, "PID:KP ", 7) == 0)
    {
        scpi_iface_p->set_kp(atof(command + 7), scpi_iface_p->context);
        send_response("OK\r\n");
    }
    else if (strncmp(command, "PID:KI ", 7) == 0)
    {
        scpi_iface_p->set_ki(atof(command + 7), scpi_iface_p->context);
        send_response("OK\r\n");
    }
    else if (strncmp(command, "PID:KD ", 7) == 0)
    {
        scpi_iface_p->set_kd(atof(command + 7), scpi_iface_p->context);
        send_response("OK\r\n");
    }
}
