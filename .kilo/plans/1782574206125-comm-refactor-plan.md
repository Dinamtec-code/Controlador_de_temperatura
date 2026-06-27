# Plan de Refactoring: Sistema de Comunicación (5 archivos)

## Contexto arquitectónico

Decisión establecida: **El registro de eventos es privado del driver**. Solo el driver puede publicar eventos directamente en `iface->event`. No existe callback `set_event` ni debe haber acceso externo.

## Incompatibilidades detectadas

### 1. `comm_driver_api.h`
- ✅ Callback `get_event` presente (línea 104)
- ✅ No hay `set_event` (acceso restringido al driver) - CORRECTO según decisión

### 2. `usart_hw.c` - Función externa no implementada
- `comm_register_iface(&usart_iface)` (línea 145) - declarada en header pero NO implementada en los 5 archivos

### 3. `usart_hw.c` - Protección en ISRs
- ISRs usan `full_protect()`/`full_unprotect()` (líneas 271, 287, 299, 302, 314, 325)
- **Usuario quiere mantener protecciones** como práctica preventiva aunque la documentación (sección 1031-1033) indica que "la única sección crítica que requiere protección externa es la captura del snapshot de eventos"

## Tareas de Refactoring

### Fase 1: `comm_driver_api.h`
- Sin cambios requeridos

### Fase 2: `circular_buffer.h` y `circular_buffer.c`
- Sin cambios - acceso directo a head/tail desde ISR es intencional (documento sección 953-955)

### Fase 3: `usart_hw.h`
- Sin cambios

### Fase 4: `usart_hw.c`
1. Mantener protecciones en ISRs según decisión del usuario
2. Agregar `__DSB()` después de `iface->rx_buffer->head = new_head;` (línea 283) para consistencia de memoria ARM

## Validación
1. Verificar `__DSB()` después de actualizaciones críticas en ISRs