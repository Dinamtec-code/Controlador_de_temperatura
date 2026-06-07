# Extensión API de Control de Interfaces de Comunicación

## Problema
`usart_hw` expone funciones como `usart_hw_start_rx()`, `usart_hw_transmit_from_system_buffer()` y `usart_hw_is_tx_ready()` que son necesarias para controlar el periférico, pero no están disponibles a través de la capa abstracta `comm_interface`. Esto obliga a `task_comm` a incluir directamente `hardware/usart_hw.h` creando acoplamiento fuerte.

## Análisis Actual

### En main.c (líneas 142-144):
```c
hrtim_hw_start();
usart_hw_start_rx();  // Llamada directa, no abstraida
```

### En task_comm.c:
- Incluye `hardware/usart_hw.h` (línea 2)
- Llama `usart_hw_is_tx_ready()` (línea 58)
- Llama `usart_hw_transmit_from_system_buffer()` (línea 60)

## Solución Propuesta

Extender `comm_interface_t` con callbacks de control que permitan a las tareas gestionar el estado de las interfaces sin conocer su implementación específica.

### Cambios Requeridos

#### 1. Core/Inc/communication/comm_interface.h
Agregar a la estructura `comm_interface_t`:
```c
void (*start_rx)(void *context);
void (*stop_rx)(void *context);
bool (*is_tx_ready)(void *context);
```

#### 2. Core/Src/hardware/usart_hw.c
- Implementar `usart_hw_start_rx_control()` como callback
- Implementar `usart_hw_stop_rx_control()` (nueva función)
- Implementar `usart_hw_is_tx_ready_control()` como callback
- Asignar los callbacks en `usart_hw_init()`

#### 3. Core/Src/communication/comm_interface.c (NUEVO)
Crear capa de enrutamiento:
```c
void comm_interface_start_rx(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->start_rx) iface->start_rx(iface->context);
}

void comm_interface_stop_rx(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->stop_rx) iface->stop_rx(iface->context);
}

bool comm_interface_is_tx_ready(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->is_tx_ready) return iface->is_tx_ready(iface->context);
    return false;
}
```

#### 4. Core/Src/tasks/task_comm.c
- Quitar include de `hardware/usart_hw.h`
- Cambiar llamadas directas por `comm_interface_*` equivalents

#### 5. Core/Src/main.c
- Cambiar `usart_hw_start_rx()` por `comm_interface_start_rx(COMM_IFACE_USART)`
- (Opcional) Mover inicialización de interfaces después del scheduler

## Alternativa Más Simple

Si solo se necesita start_rx y no el resto, se puede:

1. Agregar solo `start_rx` a `comm_interface_t`
2. Crear función `comm_interface_start_rx(id)` en un nuevo archivo comm_interface.c
3. Mantener `is_tx_ready` y `transmit_from_system_buffer` como funciones estáticas en task_comm que llama al callback `send` de la interfaz

## Archivos a Modificar

| Archivo | Acción |
|---------|--------|
| Core/Inc/communication/comm_interface.h | Agregar callbacks de control y funciones de envío |
| Core/Src/communication/comm_buffers.c | Agregar funciones de enrutamiento |
| Core/Src/hardware/usart_hw.c | Implementar callbacks y `usart_hw_stop_rx()` |
| Core/Src/tasks/task_comm.c | Usar nueva API |
| Core/Src/tasks/task_system.c | Usar `comm_interface_send()` |
| Core/Src/main.c | Usar `comm_interface_start_rx()` |

### Nota sobre scpi_parser.c
El archivo scpi_parser.c línea 92 tiene fallback `usart_hw_send_str(resp)` cuando `output_iface` no está configurado. Esto debería mantenerse por compatibilidad o eliminarse si no es necesario.

### Nota sobre stm32f3xx_it.c
El archivo de interrupciones llama a `usart_hw_idle_handler()` - esta función NO debe abstraerse ya que es ISR y debe tener latencia mínima. Se mantiene como include directo.

## Implementación Detallada

### comm_interface.h - Agregar a struct comm_interface:
```c
void (*start_rx)(void *context);      /**< Iniciar recepción (habilita DMA/interrupts) */
void (*stop_rx)(void *context);       /**< Detener recepción */
bool (*is_tx_ready)(void *context);   /**< Verificar si interfaz lista para TX */
void (*start_tx)(void *context);      /**< Transmitir desde buffer del sistema */
```

### comm_buffers.c - Agregar funciones de enrutamiento (ya tiene `comm_register_interface`, `comm_get_interface`):
```c
// Nuevas funciones a agregar después de comm_get_interface():
void comm_interface_start_rx(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->start_rx) iface->start_rx(iface->context);
}

void comm_interface_stop_rx(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->stop_rx) iface->stop_rx(iface->context);
}

bool comm_interface_is_tx_ready(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->is_tx_ready) return iface->is_tx_ready(iface->context);
    return false;
}

void comm_interface_start_tx(comm_interface_id_t id) {
    comm_interface_t *iface = comm_get_interface(id);
    if (iface && iface->start_tx) iface->start_tx(iface->context);
}
```

### comm_interface.h - Declaraciones públicas a agregar:
```c
/**
 * @brief Iniciar recepción en la interfaz especificada.
 * @param id Identificador de la interfaz.
 */
void comm_interface_start_rx(comm_interface_id_t id);

/**
 * @brief Detener recepción en la interfaz especificada.
 * @param id Identificador de la interfaz.
 */
void comm_interface_stop_rx(comm_interface_id_t id);

/**
 * @brief Verificar si la interfaz está lista para transmitir.
 * @param id Identificador de la interfaz.
 * @return true si puede iniciar transmisión, false en caso contrario.
 */
bool comm_interface_is_tx_ready(comm_interface_id_t id);

/**
 * @brief Iniciar transmisión desde el buffer del sistema.
 * @param id Identificador de la interfaz.
 */
void comm_interface_start_tx(comm_interface_id_t id);

/**
 * @brief Enviar datos directamente a una interfaz (encolado en buffer TX).
 * @param id Identificador de la interfaz.
 * @param data Puntero a los datos.
 * @param len Cantidad de bytes.
 * @return true si se encoló correctamente.
 */
bool comm_interface_send(comm_interface_id_t id, const uint8_t *data, size_t len);

### usart_hw.c - Modificar usart_hw_init para asignar callbacks:
```c
void usart_hw_init(void)
{
    usart_interface.id = COMM_IFACE_USART;
    usart_interface.context = NULL;
    usart_interface.name = "USART2";
    usart_interface.send = usart_hw_send;
    usart_interface.is_connected = usart_hw_is_connected;
    usart_interface.start_rx = usart_hw_start_rx;
    usart_interface.stop_rx = usart_hw_stop_rx;  // Nueva
    usart_interface.is_tx_ready = usart_hw_is_tx_ready;
    usart_interface.start_tx = usart_hw_transmit_from_system_buffer;  // Reusar existente
    usart_interface.rx_indication_cb = NULL;
    usart_interface.tx_complete_cb = NULL;

    comm_register_interface(&usart_interface);
}

// Nueva función a agregar:
void usart_hw_stop_rx(void)
{
    __HAL_UART_DISABLE_IT(&huart2, UART_IT_IDLE);
    HAL_UART_DMAStop(&huart2);
}
```

### task_comm.c - Quitar include usart_hw.h, usar nueva API:
```c
// Quitar: #include "hardware/usart_hw.h"
#include "communication/comm_interface.h"

void task_comm(void)
{
    // ... código existente ...
    
    if (comm_interface_is_tx_ready(COMM_IFACE_USART))
    {
        comm_interface_start_tx(COMM_IFACE_USART);
    }
}
```

### task_system.c - Usar comm_buffer_tx_put en su lugar:
```c
#include "tasks/task_system.h"
#include "communication/comm_interface.h"  // o comm_buffers.h
#include "services/error_handler.h"

void task_system(void)
{
    if (error_check(ERROR_REMOTE_RX_OVERFLOW)) {
        const char *msg = "ERR:RX_BUFFER_OVERFLOW\r\n";
        comm_interface_send(COMM_IFACE_USART, (const uint8_t*)msg, strlen(msg));
        error_clear(ERROR_REMOTE_RX_OVERFLOW);
    }
}
```

### main.c - Usar nueva API:
```c
// Cambiar: usart_hw_start_rx();
comm_interface_start_rx(COMM_IFACE_USART);
```

## Resumen

Esta extensión permite que las tareas (task_comm, task_system, u otras futuras) inicien, detengan y verifiquen el estado de las interfaces de comunicación a través de una API abstracta común. Los beneficios son:

1. **Desacoplamiento**: Las tareas no conocen el hardware específico
2. **Extensibilidad**: Se pueden agregar interfaces TCP, USB, etc. sin modificar tareas
3. **Consistencia**: API uniforme para todas las operaciones de control
4. **ISR sin abstracción**: El manejador de interrupciones mantiene baja latencia

**Nota**: Esta API fue extendida con la máquina de estados en `comm-interface-state-machine.md` para manejo automático de errores y reinicio de interfaces.

## Implementación completada

- [x] Agregar callbacks a `comm_interface_t` struct
- [x] Declarar nuevas funciones en `comm_interface.h`
- [x] Implementar funciones en `comm_buffers.c`
- [x] Asignar callbacks en `usart_hw_init()`
- [x] Crear `usart_hw_stop_rx()`
- [x] Actualizar `task_comm.c`
- [x] Actualizar `task_system.c`
- [x] Actualizar `main.c`

## Archivos modificados

| Archivo | Cambios |
|---------|---------|
| Core/Inc/communication/comm_interface.h | Agregados 4 callbacks (start_rx, stop_rx, is_tx_ready, start_tx) y 5 funciones públicas |
| Core/Src/communication/comm_buffers.c | Agregado include comm_interface.h y 5 funciones de enrutamiento |
| Core/Src/hardware/usart_hw.c | Implementado usart_hw_stop_rx() y asignados callbacks |
| Core/Src/tasks/task_comm.c | Cambiado include usart_hw.h por comm_interface.h, usado nueva API |
| Core/Src/tasks/task_system.c | Cambiado include usart_hw.h por comm_interface.h, usado comm_interface_send() |
| Core/Src/main.c | Cambiado usart_hw_start_rx() por comm_interface_start_rx() |