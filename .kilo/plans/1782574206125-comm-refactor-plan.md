# Plan de Refactoring: Sistema de Comunicación (5 archivos)

## Contexto

Refactoring estrictamente limitado a 5 archivos:
- `comm_interface.h`
- `circular_buffer.h`
- `circular_buffer.c`
- `usart_hw.h`
- `usart_hw.c`

## Problemas detectados en el estado actual

### 1. `usart_hw.c` - Race condition en ISRs

**Código actual (INCORRECTO):**
```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t data_size)
{
    // ...
    iface->protect_rx(iface);  // ISR deshabilita su propia IRQ - RACE CONDITION
    // ...
}
```

**Arquitectura documentada (sección 957-961):**
- ISR escribe en buffer (productor) - NO necesita protección para escritura simple
- La task usa protección cuando lee (consumidor)
- El `head` se actualiza en ISR, el `tail` en task

**Problema:** Llamar a `protect_rx()` desde ISR que deshabilita `USART2_IRQn` crea race condition cuando la ISR interrumpe a sí misma.

### 2. `comm_interface.h` - Declaraciones externas faltantes

`usart_hw.c` usa funciones definidas en `comm_buffers.c` (fuera del scope):
- `comm_register_interface()` - línea 125
- `comm_get_interface()` - múltiples llamadas

## Tareas de Refactoring

### Fase 1: `comm_interface.h`
Agregar extern declarations antes del `#endif`:
```c
extern void comm_register_interface(comm_iface_t *iface);
extern comm_iface_t *comm_get_interface(comm_iface_id_t id);
```

### Fase 2: `usart_hw.c`
Revertir protecciones en ISRs - según arquitectura, el productor (ISR) NO necesita protegerse:

1. `HAL_UARTEx_RxEventCallback` (líneas 258, 274): ELIMINAR llamadas a `protect_rx()`/`unprotect_rx()`
2. `HAL_UART_TxCpltCallback` (líneas 286, 289): ELIMINAR llamadas a `protect_tx()`/`unprotect_tx()`
3. `HAL_UART_ErrorCallback` (líneas 301, 312): ELIMINAR llamadas a `protect_rx()`/`unprotect_rx()`
4. Agregar `__DSB()` después de actualizar `head`/`state` para consistencia de memoria

### Fase 3: `circular_buffer.h` y `circular_buffer.c`
- No requieren cambios

## Riesgos

- Los cambios eliminan protección excesiva en ISRs, no la protección necesaria
- La task seguirá usando protección cuando lea/escriba buffers

## Validación

1. Verificar compilación de `usart_hw.c`
2. Verificar ausencia de race conditions en ISRs