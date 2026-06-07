# Refactoring: Gestión de Estados en Interfaz de Comunicación

## Problema Identificado

El diseño actual de `comm_interface_state_t` tiene limitaciones:

1. **Mutua exclusividad forzada**: El código en `task_comm.c` usa comparaciones `== COMM_STATE_RX_ACTIVE`, `== COMM_STATE_TX_BUSY`, etc., tratando el estado como valor único en lugar de bitmask
2. **Estado IDLE ambiguo**: `COMM_STATE_IDLE` no se usa en el código actual, pero su definición sugiere un estado neutro que podría coexistir con otros
3. **Separación de datos**: Los estados están en un arreglo global `interface_states[COMM_IFACE_MAX]` mientras que la interfaz está en otro arreglo, violando el encapsulamiento

## Solución Propuesta

### 1. Rediseño de Estados (Bitmask)

```c
typedef enum {
    COMM_STATE_UNINIT    = 0x00,
    COMM_STATE_ERROR     = 0x01,
    COMM_STATE_RX_ACTIVE = 0x02,
    COMM_STATE_TX_BUSY   = 0x04
} comm_interface_state_t;
```

Los estados se combinan con operaciones bitwise:
- RX + TX puededen estar activos simultáneamente (FULL-DUPLEX)
- ERROR es un flag que puede combinarse con otros estados

### 2. Nueva API de Acceso a Estados

Reemplazar `comm_interface_get_state/set_state()` con getters/setters específicos:

```c
// Getters
bool comm_interface_is_rx_active(comm_interface_t *iface);
bool comm_interface_is_tx_ready(comm_interface_t *iface);
bool comm_interface_is_error(comm_interface_t *iface);

// Setters
void comm_interface_set_rx_active(comm_interface_t *iface, bool active);
void comm_interface_set_tx_busy(comm_interface_t *iface, bool busy);
void comm_interface_set_error(comm_interface_t *iface, bool error);
```

### 3. Optimización de Arquitectura de Datos

Mover el estado dentro de `comm_interface_t`:

```c
struct comm_interface
{
    comm_interface_id_t id;
    void *context;
    const char *name;
    comm_interface_state_t state;  // Nuevo campo interno
    
    // Callbacks...
    bool (*send)(const uint8_t *data, size_t len);
    bool (*is_connected)(void *context);
    void (*start_rx)(void *context);
    void (*stop_rx)(void *context);
    bool (*is_tx_ready)(void *context);
    bool (*start_tx)(void *context);
    
    comm_rx_indication_callback_t rx_indication_cb;
    comm_tx_complete_callback_t tx_complete_cb;
};
```

Eliminar `interface_states[]` del módulo comm_buffers.

## Archivos a Modificar

| Archivo | Cambios |
|---------|---------|
| `Core/Inc/communication/comm_interface.h` | Redefinir enum, agregar campo state, nueva API |
| `Core/Src/communication/comm_buffers.c` | Eliminar arreglo states[], implementar nueva API |
| `Core/Src/tasks/task_comm.c` | Actualizar FSM para usar nueva API |
| `Core/Src/hardware/usart_hw.c` | Eliminar llamadas a `comm_interface_set_state(COMM_STATE_*)`, usar setters |

## Implementación Detallada

### comm_interface.h - Nuevas definiciones

```c
typedef enum {
    COMM_STATE_NONE      = 0x00,
    COMM_STATE_ERROR     = 0x01,
    COMM_STATE_RX_ACTIVE = 0x02,
    COMM_STATE_TX_BUSY   = 0x04
} comm_interface_state_t;

// Máscaras para operaciones
#define COMM_STATE_MASK_ERROR     0x01
#define COMM_STATE_MASK_RX_ACTIVE 0x02
#define COMM_STATE_MASK_TX_BUSY   0x04

// Getters (nueva API)
bool comm_interface_is_rx_active(comm_interface_t *iface);
bool comm_interface_is_tx_busy(comm_interface_t *iface);
bool comm_interface_has_error(comm_interface_t *iface);

// Setters (nueva API)
void comm_interface_set_rx_active(comm_interface_t *iface, bool active);
void comm_interface_set_tx_busy(comm_interface_t *iface, bool busy);
void comm_interface_set_error(comm_interface_t *iface, bool error);

// API antigua (deprecada, mantener por compatibilidad)
void comm_interface_set_state(comm_interface_id_t id, comm_interface_state_t state);
comm_interface_state_t comm_interface_get_state(comm_interface_id_t id);
```

### comm_buffers.c - Implementación de nuevos setters

```c
bool comm_interface_is_rx_active(comm_interface_t *iface) {
    return iface && (iface->state & COMM_STATE_RX_ACTIVE) != 0;
}

bool comm_interface_is_tx_busy(comm_interface_t *iface) {
    return iface && (iface->state & COMM_STATE_TX_BUSY) != 0;
}

bool comm_interface_has_error(comm_interface_t *iface) {
    return iface && (iface->state & COMM_STATE_ERROR) != 0;
}

void comm_interface_set_rx_active(comm_interface_t *iface, bool active) {
    if (iface) {
        if (active) {
            iface->state |= COMM_STATE_RX_ACTIVE;
        } else {
            iface->state &= ~COMM_STATE_RX_ACTIVE;
        }
    }
}

void comm_interface_set_tx_busy(comm_interface_t *iface, bool busy) {
    if (iface) {
        if (busy) {
            iface->state |= COMM_STATE_TX_BUSY;
        } else {
            iface->state &= ~COMM_STATE_TX_BUSY;
        }
    }
}

void comm_interface_set_error(comm_interface_t *iface, bool error) {
    if (iface) {
        if (error) {
            iface->state |= COMM_STATE_ERROR;
        } else {
            iface->state &= ~COMM_STATE_ERROR;
        }
    }
}
```

### task_comm.c - FSM actualizado

```c
void task_comm(void)
{
    static bool output_iface_set = false;

    if (!output_iface_set) {
        scpi_output_interface_t out_iface = {
            .send_response = send_response_to_interface,
            .context = NULL};
        scpi_set_output_interface(&out_iface);
        output_iface_set = true;
    }

    comm_interface_t *iface = comm_get_interface(COMM_IFACE_USART);
    
    if (!iface || !comm_interface_is_rx_active(iface)) {
        comm_interface_start_rx(COMM_IFACE_USART);
        comm_interface_set_rx_active(iface, true);
        return;
    }

    if (comm_interface_has_error(iface)) {
        comm_interface_reset(COMM_IFACE_USART);
        return;
    }

    process_rx_data();

    if (comm_buffer_tx_count(COMM_IFACE_USART) > 0) {
        comm_interface_set_tx_busy(iface, true);
    }

    if (comm_interface_is_tx_busy(iface) && comm_interface_is_tx_ready(COMM_IFACE_USART)) {
        if (!comm_interface_start_tx(COMM_IFACE_USART)) {
            comm_interface_set_tx_busy(iface, false);
        }
    }
}
```

### usart_hw.c - Callbacks actualizados

```c
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        comm_interface_t *iface = comm_get_interface(COMM_IFACE_USART);
        comm_interface_set_tx_busy(iface, false);
        
        // Check if more data to send
        if (comm_buffer_tx_count(COMM_IFACE_USART) > 0) {
            comm_interface_start_tx(COMM_IFACE_USART);
        }
        
        if (usart_interface.tx_complete_cb) {
            usart_interface.tx_complete_cb(COMM_IFACE_USART);
        }
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        comm_interface_t *iface = comm_get_interface(COMM_IFACE_USART);
        comm_interface_set_error(iface, true);
    }
}
```

## Observaciones de Bug Adicional

En `usart_hw.c:122-125`, el callback `tx_complete_cb` se llama desde `usart_interface` (variable estática local) en lugar de desde la interfaz obtenida del registro. Esto es inconsistente y podría fallar si `usart_interface` no está sincronizado con la registrada.

```c
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    if (huart->Instance == USART2) {
        comm_interface_t *iface = comm_get_interface(COMM_IFACE_USART);
        if (iface && iface->tx_complete_cb) {  // Usar iface registrada
            iface->tx_complete_cb(COMM_IFACE_USART);
        }
    }
}
```

## Estado de la Implementación

- [x] Redefinir enum como bitmask
- [x] Agregar campo state en comm_interface_t
- [x] Eliminar interface_states[] del módulo comm_buffers
- [x] Implementar getters/setters específicos
- [x] Actualizar task_comm.c FSM
- [x] Actualizar usart_hw.c callbacks
- [x] Corregir bug: usar iface registrada en lugar de variable local estática

## Beneficios

1. **Full-duplex explícito**: RX y TX pueden estar activos simultáneamente
2. **Mejor encapsulamiento**: Cada interfaz gestiona su propio estado
3. **FSM más robusto**: Los estados son independientes y se pueden combinar
4. **Escalabilidad**: Agregar nuevos estados no rompe el diseño actual
5. **Detección de error no intrusiva**: El flag ERROR puede coexistir con otros estados