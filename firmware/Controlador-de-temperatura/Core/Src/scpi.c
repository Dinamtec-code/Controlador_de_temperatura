#include "scpi.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include "main.h"

extern float temperatureSetpoint;
extern float pidKp, pidKi, pidKd;

static const scpi_interface_t *scpi_iface = NULL;
static const scpi_input_interface_t *input_iface = NULL;
static const scpi_output_interface_t *output_iface = NULL;

void scpi_init(const scpi_interface_t *iface)
{
    scpi_iface = iface;
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
}

void scpi_process(void)
{
    if (!scpi_iface || !input_iface)
        return;

    uint8_t *buffer;
    size_t len;
    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

    if (!input_iface->get_rx_buffer)
        return;
    if (input_iface->get_rx_buffer(&buffer, &len) == 0)
        return;
    if (len == 0)
        return;

    HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);

    if (strncmp((char *)buffer, "*IDN?", 5) == 0)
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
        send_response("TEMPCTRL,STM32F334,1.0\r\n");
    }
    else if (strncmp((char *)buffer, "*CLS", 4) == 0)
    {
        send_response("OK\r\n");
    }
    else if (strncmp((char *)buffer, "*RST", 4) == 0)
    {
        temperatureSetpoint = 25.0f;
        pidKp = 1.0f;
        pidKi = 0.5f;
        pidKd = 0.1f;
        send_response("OK\r\n");
    }
    else if (strncmp((char *)buffer, "MEAS:TEMP?", 10) == 0)
    {
        float temp = scpi_iface->get_temp(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.2f\r\n", temp);
        send_response(resp);
    }
    else if (strncmp((char *)buffer, "TEMP:SP?", 8) == 0)
    {
        float sp = scpi_iface->get_sp(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.2f\r\n", sp);
        send_response(resp);
    }
    else if (strncmp((char *)buffer, "TEMP:SP ", 8) == 0)
    {
        float val = atof((char *)buffer + 8);
        if (val >= 0 && val <= 200)
        {
            scpi_iface->set_sp(val, scpi_iface->context);
            send_response("OK\r\n");
        }
        else
        {
            send_response("ERR\r\n");
        }
    }
    else if (strncmp((char *)buffer, "PID:KP?", 7) == 0)
    {
        float kp = scpi_iface->get_kp(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", kp);
        send_response(resp);
    }
    else if (strncmp((char *)buffer, "PID:KI?", 7) == 0)
    {
        float ki = scpi_iface->get_ki(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", ki);
        send_response(resp);
    }
    else if (strncmp((char *)buffer, "PID:KD?", 7) == 0)
    {
        float kd = scpi_iface->get_kd(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", kd);
        send_response(resp);
    }
    else if (strncmp((char *)buffer, "PID:KP ", 7) == 0)
    {
        scpi_iface->set_kp(atof((char *)buffer + 7), scpi_iface->context);
        send_response("OK\r\n");
    }
    else if (strncmp((char *)buffer, "PID:KI ", 7) == 0)
    {
        scpi_iface->set_ki(atof((char *)buffer + 7), scpi_iface->context);
        send_response("OK\r\n");
    }
    else if (strncmp((char *)buffer, "PID:KD ", 7) == 0)
    {
        scpi_iface->set_kd(atof((char *)buffer + 7), scpi_iface->context);
        send_response("OK\r\n");
    }
    else
    {
        HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
    }

    if (input_iface->consume_input)
    {
        input_iface->consume_input();
    }
}