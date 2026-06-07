# Arquitectura de Stack de Comunicación

## Objetivo
Crear una arquitectura de comunicación modular, robusta y extensible que soporte múltiples interfaces físicas (USART, WiFi, Ethernet, USB, etc.) con buffers únicos del sistema y callbacks estandarizados.

## Componentes

### 1. HAL de ST (generado por CubeMX)
- **Ubicación**: `usart.c` (CubeMX generado)
- **Responsabilidad**: Configuración UART del HAL, handlers DMA/UART
- **Nota**: NO tocar - archivo generado automáticamente

### 2. Capa de Abstracción por Periférico
- **Ubicación**: CARPETA `hardware/` (contiene `usart_hw.c`, `adc_hw.c`, etc.)
- **Responsabilidad**: 
  - Adaptar el HAL al API común de comunicación
  - Gestionar colas de transmisión propias
  - Llenar/vaciar los buffers del sistema mediante callbacks

### 3. Buffers del Sistema
- **Ubicación**: `communication/comm_buffers.{h,c}`
- **Característica**: Buffers únicos `circular_buffer_t` para RX/TX
- **Función**: Almacenar datos pendientes de procesamiento/por enviar

### 4. Tarea de Comunicación
- **Ubicación**: `tasks/task_comm.{h,c}` (refactorizar)
- **Responsabilidad**:
  - Registrar interfaces activas
  - Procesar datos del buffer RX → parser
  - Dirigir respuestas del parser → buffer TX del destino

### 5. Parser/Protocolo
- **Opción actual**: Sistema SCPI propio (`services/scpi_parser.c`)
- **Alternativa**: SCPI parser de Jay Breuer (https://github.com/j123b567/scpi-parser)
- **Extensibilidad**: Soporte para otros parsers futuros

## API Propuesta

### Interface Abstraction API
```c
typedef struct {
    void* context;
    const char* name;
    
    // Funciones de la capa de abstracción
    void (*init)(void* context);
    void (*deinit)(void* context);
    bool (*send)(const uint8_t* data, size_t len);  // Llena buffer TX del sistema
    bool (*receive)(uint8_t* data, size_t* len);   // Vacía buffer RX del sistema
    bool (*is_connected)(void* context);
} comm_interface_t;

// Callbacks que la capa HAL llama
typedef void (*comm_rx_callback_t)(void* context, uint8_t* data, size_t len);
typedef void (*comm_tx_complete_callback_t)(void* context);
```

### Registro de Interfaces
```c
// task_comm.c
void comm_register_interface(comm_interface_t* iface);
void comm_unregister_interface(comm_interface_t* iface);
```

## Arquitectura Actual (Corregida)

```
Drivers/STM32F3xx_HAL_Driver/  ← HAL de ST (bajo nivel)
        ↓
hardware/usart_hw.c/h        ← Capa de abstracción USART (ya existe)
        ↓
rx_circular_buffer (único)   ← Buffer del sistema
        ↓
tasks/task_comm.c            ← Tarea de comunicación
        ↓
services/scpi_parser.c       ← Parser (llama directo a usart_hw_send_str)
```

**Problema**: El parser llama **directamente** a `usart_hw_send_str()`, creando acoplamiento fuerte. No hay soporte para múltiples interfaces.

## Arquitectura Propuesta

```
Drivers/STM32F3xx_HAL_Driver/  ← HAL de ST (bajo nivel, NO tocar)
        ↓
usart.c (CubeMX)              ← Configuración UART (NO tocar)
        ↓
hardware/usart_hw.c/h          ← Capa de abstracción USART (REFACTOR)
        ↓
communication/comm_buffers.{h,c}  ← Buffers únicos del sistema (RX/TX por interface)
        ↓
tasks/task_comm.{h,c}         ← Tarea orquestadora
        ↓
services/scpi_parser.c        ← Parser (sin acoplamiento a hardware)
```

### Flujo de Datos RX:
1. ISR (USART2) detecta IDLE → llama `usart_hw_idle_handler()`
2. `usart_hw.c` lee `dma_uart_rx_buffer`, copia a `comm_buffer_rx[USART]`
3. `task_comm` procesa buffer USART → llama al parser
4. Parser genera respuesta → escribe en `comm_buffer_tx[USART]`
5. `task_comm` llama a `usart_hw_send_buf()` para transmitir

### Flujo de Datos TX:
La respuesta se dirige usando la misma interfaz que recibió el comando.

## API Estándar de Interfaces

```c
// Core/Inc/communication/comm_interface.h
typedef enum {
    COMM_IFACE_USART = 0,
    COMM_IFACE_TCP,
    COMM_IFACE_USB,
    COMM_IFACE_MAX
} comm_interface_id_t;

typedef struct {
    comm_interface_id_t id;
    void* context;
    
    // Funciones de la capa de abstracción
    bool (*send)(const uint8_t* data, size_t len);
    bool (*is_connected)(void);
} comm_interface_t;

// Registro (llamado desde main.c)
void comm_register_interface(comm_interface_t* iface);
```

### Integración con usart_hw
`usart_hw.c` implementará la API `comm_interface_t` y registrará USART como interface válida al inicializar.

## Definición de Callbacks

### `comm_rx_indication(interface_id, data, len)`
- **Propósito**: Notificar al sistema de comunicación que hay datos nuevos recibidos.
- **Origen**: Llamado desde la capa `hardware/` (ej: `usart_hw_idle_handler()`) después de copiar datos del DMA a cb_buffer.
- **Consumidor**: `task_comm` procesa el buffer mediante el parser.
- **Flujo**: ISR → `usart_hw_idle_handler()` lee DMA → `cb_put()` datos al buffer → `comm_rx_indication(COMM_IFACE_USART, NULL, len)` (len indica cuántos bytes nuevos).

Nota: `data=NULL` porque los datos ya están en el cb_buffer. El `len` es para que `task_comm` sepa cuántos procesar de una vez.

### `comm_tx_complete(interface_id)`
- **Propósito**: Notificar que la transmisión DMA/Blocking terminó.
- **Origen**: Llamado desde `HAL_UART_TxCpltCallback()` cuando el UART finaliza TX.
- **Consumidor**: `task_comm` para liberar recursos o iniciar siguiente transmisión.
- **Flujo**: DMA envía datos → `HAL_UART_TxCpltCallback()` → `comm_tx_complete(COMM_IFACE_USART)` → `task_comm` verifica si hay más datos pendientes en `comm_buffer_tx[USART]`.

## Direccionamiento de Respuesta (Propuesta)

El parser actual llama `usart_hw_send_str()` directamente. Con la nueva arquitectura:

### Opción A (Recomendada): task_comm como orquestador
```c
typedef struct {
    comm_interface_id_t origin_iface;
    char data[64];
    size_t len;
} comm_rx_message_t;

void task_comm(void) {
    comm_rx_message_t msg;
    // 1. Lee del buffer RX correspondiente
    while (comm_buffer_read_msg(&msg) == MSG_OK) {
        // 2. Procesa con el parser, pasando el origen
        scpi_process_line_with_origin(msg.data, msg.origin_iface);
    }
    
    // 3. Envía respuestas desde buffers TX
    comm_interface_id_t iface;
    while (comm_get_pending_tx_iface(&iface)) {
        comm_transmit_from_buffer(iface);
    }
}
```

### Opción B: Parser con callback
El parser recibe un callback `send_response(const char* resp, comm_interface_id_t dest)` y escribe directamente al buffer TX del destino.

**¿Cuál prefieres? (Recomiendo Opción A para desacoplar parser del sistema de buffers)**.

### Mensajes con Delimitadores
Para SCPI necesitamos delimitar líneas. El cb_buffer puede usarse con un wrapper que:
- Acumule bytes hasta encontrar `\n` o `\r\n`
- Expropie mensajes completos a `task_comm`
- Gestione timeout para mensajes incompletos

## Respuestas Recibidas

- [x] **Punto 1**: Enum para identificador de interfaz ✅
- [x] **Punto 2**: Buffer separado por interfaz ✅  
- [x] **Punto 3**: No interfaces simultáneas por ahora ✅
- [x] **Punto 5**: Mantener parser actual ✅
- [x] **Direccionamiento de respuestas**: Opción A (task_comm orquestador) - Parser usa output interface abstraction ✅

## Archivos Creados

### Crear:
- [x] `Core/Inc/communication/comm_interface.h` - API estándar de interfaces
- [x] `Core/Inc/communication/comm_buffers.h` - Buffers únicos del sistema
- [x] `Core/Src/communication/comm_buffers.c` - Gestión de buffers
- [x] `Core/Inc/communication/parser_types.h` - Tipos para parser callbacks (no requerido - usa scpi_output_interface_t)

### Modificar:
- [x] `Core/Inc/hardware/usart_hw.h` - Refactorizado, ahora implementa comm_interface_t
- [x] `Core/Src/hardware/usart_hw.c` - Implementa callbacks, usa comm_buffers
- [x] `Core/Src/tasks/task_comm.c` - Usa nueva arquitectura con buffers
- [x] `Core/Src/services/scpi_parser.c` - Usa output interface abstraction
- [x] `Core/Src/main.c` - Inicializa comm_buffers, registra usart_hw como interface

## Notas de Implementación

### Implementada Opción B (Parser con callback):
El parser usa `scpi_output_interface_t` que llama a `comm_buffer_tx_put()` en el callback. Esto simplifica el flujo ya que el parser no necesita conocer el origen de la respuesta.

### Próximos pasos:
- Verificar compilación
- Testear flujo de datos USART → task_comm → parser → respuesta
- Si agregan interfaces TCP/USB, implementar capa hardware correspondiente