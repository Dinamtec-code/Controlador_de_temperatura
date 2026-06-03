#ifndef SCPI_PARSER_H
#define SCPI_PARSER_H

#include <stdint.h>
#include <stddef.h>

typedef void (*scpi_response_callback_t)(const char* resp, void* context);
typedef float (*scpi_get_sensor_callback_t)(void* context);
typedef void (*scpi_set_float_callback_t)(float value, void* context);

typedef struct {
    scpi_get_sensor_callback_t get_temp;
    scpi_get_sensor_callback_t get_sp;
    scpi_set_float_callback_t set_sp;
    scpi_set_float_callback_t set_kp;
    scpi_set_float_callback_t set_ki;
    scpi_set_float_callback_t set_kd;
    scpi_get_sensor_callback_t get_kp;
    scpi_get_sensor_callback_t get_ki;
    scpi_get_sensor_callback_t get_kd;
    void* context;
} scpi_interface_t;

void scpi_init(const scpi_interface_t* iface);
void scpi_process_line(const char* command);

#endif