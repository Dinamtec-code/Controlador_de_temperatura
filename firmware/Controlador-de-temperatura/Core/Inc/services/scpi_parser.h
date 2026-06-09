#ifndef SCPI_PARSER_H
#define SCPI_PARSER_H

#include <stdint.h>
#include <stddef.h>

typedef void (*scpi_response_callback_t)(const char *resp, void *context);
typedef float (*scpi_get_sensor_callback_t)(void *context);
typedef void (*scpi_set_float_callback_t)(float value, void *context);
typedef bool (*scpi_get_output_callback_t)(int channel, void *context);
typedef void (*scpi_set_output_callback_t)(int channel, bool value, void *context);

typedef struct
{
    scpi_get_sensor_callback_t get_temp;
    scpi_get_sensor_callback_t get_sp;
    scpi_set_float_callback_t set_sp;
    scpi_set_float_callback_t set_kp;
    scpi_set_float_callback_t set_ki;
    scpi_set_float_callback_t set_kd;
    scpi_get_sensor_callback_t get_kp;
    scpi_get_sensor_callback_t get_ki;
    scpi_get_sensor_callback_t get_kd;
    scpi_get_output_callback_t get_out;
    scpi_get_output_callback_t get_output;
    scpi_set_output_callback_t set_output;
    void *context;
} scpi_interface_t;

typedef struct
{
    void *context;
    size_t (*get_rx_buffer)(uint8_t **buffer, size_t *len);
    void (*consume_input)(void);
} scpi_input_interface_t;

typedef struct
{
    void (*send_response)(const char *resp, void *context);
    void *context;
} scpi_output_interface_t;

void scpi_init(void);
void scpi_set_input_interface(const scpi_input_interface_t *input_iface);
void scpi_set_output_interface(const scpi_output_interface_t *output_iface);
void scpi_process_line(const char *command);

#endif