# Máquina de Estados para Gestión de Interfaces de Comunicación

## Problema
La inicialización de `start_rx` en `main.c` no es suficiente. Si la recepción se detiene por error (overflow, desconexión, error de DMA), no hay mecanismo para reiniciarla automáticamente. La gestión del estado de las interfaces debería estar centralizada en la tarea de comunicación.

## Propuesta

Crear una máquina de estados (FSM) en `task_comm` que gestione el ciclo de vida de las interfaces.

### Estados de la interfaz

```c
typedef enum {
    COMM_STATE_UNINIT,      // No inicializada
    COMM_STATE_IDLE,        // Lista, esperando datos
    COMM_STATE_RX_ACTIVE,   // Recepción activa
    COMM_STATE_TX_BUSY,     // Transmitiendo
    COMM_STATE_ERROR        // Error, necesita reset
} comm_interface_state_t;
```

### API extendida necesaria

Agregar a `comm_interface.h`:
```c
// Función para obtener estado actual
comm_interface_state_t comm_interface_get_state(comm_interface_id_t id);

// Función para forzar reset de interfaz
void comm_interface_reset(comm_interface_id_t id);
```

### FSM en task_comm

```c
void task_comm(void)
{
    static comm_interface_state_t state = COMM_STATE_UNINIT;
    static bool output_iface_set = false;

    switch (state) {
        case COMM_STATE_UNINIT:
            scpi_init();
            scpi_set_output_interface(&out_iface);
            comm_interface_start_rx(COMM_IFACE_USART);
            state = COMM_STATE_RX_ACTIVE;
            break;

        case COMM_STATE_RX_ACTIVE:
            // Procesar datos RX recibidos
            process_rx_data();
            // Verificar si hay TX pendiente
            if (comm_buffer_tx_count(COMM_IFACE_USART) > 0) {
                state = COMM_STATE_TX_BUSY;
            }
            break;

        case COMM_STATE_TX_BUSY:
            if (comm_interface_is_tx_ready(COMM_IFACE_USART)) {
                if (!comm_interface_start_tx(COMM_IFACE_USART)) {
                    state = COMM_STATE_RX_ACTIVE;  // Buffer vacío o error
                }
            }
            break;

        case COMM_STATE_ERROR:
            // Resetear interfaz
            comm_interface_stop_rx(COMM_IFACE_USART);
            comm_interface_start_rx(COMM_IFACE_USART);
            state = COMM_STATE_RX_ACTIVE;
            break;
    }
}
```

### Callback de error para detección

El HAL puede notificar errores a través de callback:

```c
// En usart_hw.c
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // Notificar a task_comm que hubo error
        comm_interface_set_error(COMM_IFACE_USART);
    }
}
```

## Archivos a Modificar

| Archivo | Acción |
|---------|--------|
| Core/Inc/communication/comm_interface.h | Agregar estado y funciones de gestión |
| Core/Src/communication/comm_buffers.c | Implementar gestión de estado por interfaz |
| Core/Src/hardware/usart_hw.c | Agregar HAL_UART_ErrorCallback si no existe |
| Core/Src/tasks/task_comm.c | Implementar FSM de gestión |
| Core/Src/main.c | Quitar start_rx de main, inicializar FSM |

## Flujo de datos con FSM

```
main.c                    task_comm.c                    usart_hw.c
   │                          │                            │
   │  init interfaces         │                            │
   │─────────────────────────>                             │
   │                          │                            │
   │                          │  start_rx()                │
   │                          │───────────────────────────>│
   │                          │  ┌──────────────┐          │
   │                          │  │ RX DMA OK    │          │
   │                          │  └──────────────┘          │
   │                          │                            │
   │<─── datos USART ────────│                            │
   │  (ISR)                 │                            │
   │                          │  Procesar buffer           │
   │                          │  TX? ─────────────────────>│
   │                          │                            │
   │                          │  HAL_UART_TxCpltCallback   │
   │                          │  (notifica tx_complete)    │
   │<─────────────────────────│                            │
```

## Precauciones

- La FSM debe ser determinista y sin bloqueos
- Los callbacks ISR deben ser mínimos, solo marcar eventos
- Usar variables estáticas para estado entre llamadas

## Detalles de Implementación

### Estado por interfaz

```c
// En comm_buffers.c - array de estados
static volatile comm_interface_state_t interface_states[COMM_IFACE_MAX];

void comm_interface_set_state(comm_interface_id_t id, comm_interface_state_t state) {
    if (id < COMM_IFACE_MAX) {
        interface_states[id] = state;
    }
}

comm_interface_state_t comm_interface_get_state(comm_interface_id_t id) {
    if (id < COMM_IFACE_MAX) {
        return interface_states[id];
    }
    return COMM_STATE_UNINIT;
}
```

### Callback de transmisión completada

```c
// En usart_hw.c - ya existe HAL_UART_TxCpltCallback
// Agregar notificación de vuelta al estado RX
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        // Volver al estado RX después de TX
        comm_interface_set_state(COMM_IFACE_USART, COMM_STATE_RX_ACTIVE);
        
        if (usart_interface.tx_complete_cb) {
            usart_interface.tx_complete_cb(COMM_IFACE_USART);
        }
    }
}
```

### FSM integrada en task_comm

```c
void task_comm(void)
{
    static comm_interface_state_t state = COMM_STATE_UNINIT;
    static bool output_iface_set = false;
    
    if (!output_iface_set) {
        scpi_output_interface_t out_iface = {
            .send_response = send_response_to_interface,
            .context = NULL};
        scpi_set_output_interface(&out_iface);
        output_iface_set = true;
    }

    switch (state) {
        case COMM_STATE_UNINIT:
            comm_interface_start_rx(COMM_IFACE_USART);
            state = COMM_STATE_RX_ACTIVE;
            break;

        case COMM_STATE_RX_ACTIVE:
            process_rx_data();
            if (comm_buffer_tx_count(COMM_IFACE_USART) > 0) {
                state = COMM_STATE_TX_BUSY;
            }
            break;

        case COMM_STATE_TX_BUSY:
            if (comm_interface_is_tx_ready(COMM_IFACE_USART)) {
                if (comm_interface_start_tx(COMM_IFACE_USART)) {
                    // El callback de TX completado pondrá estado a RX_ACTIVE
                } else {
                    // Buffer vacío, volver a RX
                    state = COMM_STATE_RX_ACTIVE;
                }
            }
            break;

        case COMM_STATE_ERROR:
            comm_interface_stop_rx(COMM_IFACE_USART);
            comm_interface_start_rx(COMM_IFACE_USART);
            state = COMM_STATE_RX_ACTIVE;
            break;
    }
}
```

### Integración con error_handler

La FSM debe también manejar errores del sistema como buffer overflow.

## Implementación completada

- [x] Agregar `comm_interface_state_t` enum en comm_interface.h
- [x] Agregar array de estados en comm_buffers.c
- [x] Implementar `comm_interface_set_state()` y `comm_interface_get_state()`
- [x] Implementar `comm_interface_reset()`
- [x] Agregar `comm_buffer_tx_count()` en comm_buffers.h/c
- [x] Modificar HAL_UART_TxCpltCallback para notificar estado
- [x] Agregar HAL_UART_ErrorCallback para manejo de errores
- [x] Implementar FSM en task_comm.c
- [x] Quitar start_rx de main.c (la FSM lo iniciará)
- [x] Compilación exitosa - FLASH 68.52%, RAM 61.13%

## Archivos modificados

| Archivo | Cambios |
|---------|---------|
| Core/Inc/communication/comm_interface.h | Agregado enum comm_interface_state_t y 3 nuevas funciones |
| Core/Src/communication/comm_buffers.c | Agregado array de estados, 3 nuevas funciones, comm_buffer_tx_count |
| Core/Src/hardware/usart_hw.c | Agregado HAL_UART_ErrorCallback, modificado TxCpltCallback |
| Core/Src/tasks/task_comm.c | Implementada FSM con estados UNINIT/RX_ACTIVE/TX_BUSY/ERROR |
| Core/Src/tasks/task_system.c | Simplificado (errores ahora en task_comm) |
| Core/Src/main.c | Quitado comm_interface_start_rx(), FSM lo inicializa |