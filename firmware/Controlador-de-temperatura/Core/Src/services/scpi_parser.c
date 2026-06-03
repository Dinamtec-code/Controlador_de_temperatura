#include "services/scpi_parser.h"
#include "hardware/usart_hw.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const scpi_interface_t *scpi_iface = NULL;

void scpi_init(const scpi_interface_t *iface)
{
    scpi_iface = iface;
}

static void send_response(const char *resp)
{
    usart_hw_send_str(resp);
}

void scpi_process_line(const char* command)
{
    if (!scpi_iface) {
        return;
    }

    if (strncmp(command, "*IDN?", 5) == 0) {
        send_response("TEMPCTRL,STM32F334,1.0\r\n");
    }
    else if (strncmp(command, "*CLS", 4) == 0) {
        send_response("OK\r\n");
    }
    else if (strncmp(command, "*RST", 4) == 0) {
        if (scpi_iface->set_sp) {
            scpi_iface->set_sp(25.0f, scpi_iface->context);
        }
        if (scpi_iface->set_kp) {
            scpi_iface->set_kp(1.0f, scpi_iface->context);
        }
        if (scpi_iface->set_ki) {
            scpi_iface->set_ki(0.5f, scpi_iface->context);
        }
        if (scpi_iface->set_kd) {
            scpi_iface->set_kd(0.1f, scpi_iface->context);
        }
        send_response("OK\r\n");
    }
    else if (strncmp(command, "MEAS:TEMP?", 10) == 0) {
        float temp = scpi_iface->get_temp(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.2f\r\n", temp);
        send_response(resp);
    }
    else if (strncmp(command, "TEMP:SP?", 8) == 0) {
        float sp = scpi_iface->get_sp(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.2f\r\n", sp);
        send_response(resp);
    }
    else if (strncmp(command, "TEMP:SP ", 8) == 0) {
        float val = atof(command + 8);
        if (val >= 0 && val <= 200) {
            scpi_iface->set_sp(val, scpi_iface->context);
            send_response("OK\r\n");
        } else {
            send_response("ERR\r\n");
        }
    }
    else if (strncmp(command, "PID:KP?", 7) == 0) {
        float kp = scpi_iface->get_kp(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", kp);
        send_response(resp);
    }
    else if (strncmp(command, "PID:KI?", 7) == 0) {
        float ki = scpi_iface->get_ki(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", ki);
        send_response(resp);
    }
    else if (strncmp(command, "PID:KD?", 7) == 0) {
        float kd = scpi_iface->get_kd(scpi_iface->context);
        char resp[32];
        snprintf(resp, sizeof(resp), "%.4f\r\n", kd);
        send_response(resp);
    }
    else if (strncmp(command, "PID:KP ", 7) == 0) {
        scpi_iface->set_kp(atof(command + 7), scpi_iface->context);
        send_response("OK\r\n");
    }
    else if (strncmp(command, "PID:KI ", 7) == 0) {
        scpi_iface->set_ki(atof(command + 7), scpi_iface->context);
        send_response("OK\r\n");
    }
    else if (strncmp(command, "PID:KD ", 7) == 0) {
        scpi_iface->set_kd(atof(command + 7), scpi_iface->context);
        send_response("OK\r\n");
    }
}