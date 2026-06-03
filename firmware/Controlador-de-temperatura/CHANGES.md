# Controlador de Temperatura - Firmware STM32

## Mejoras Realizadas

### main.c
- **Simplificación de State Machines**: Reemplazadas `parseContext_t`/`commContext_t` con `TaskState_t` (STATE_IDLE, STATE_PROCESSING, STATE_ERROR)
- **Corrección de conversiones de tipo**: `atof()` → `strtol()` con validación de entrada y bounds checking
- **Validación de rangos PWM**: Rango 0-63999 (HRTIM_PERIOD)
- **Implementado comando F**: Configura frecuencia HRTIM (ej: F,150000)
- **Eliminada variable `frecuency`** sin uso

### usart.c
- **Buffer circular inicializado correctamente**: `BufferStartOffset=0`, `BufferEndOffset=0`
- **Protección de race conditions**: `sendChar()` y `__io_putchar()` usan `__disable_irq()`/`__enable_irq()`
- **Mejorado manejo de buffer**: Cambiado `Error_Handler()` por retorno `HAL_ERROR` en overflow

### hrtim.c
- **Corregido TIMER_B duplicado**: Línea 133 tenía configuración duplicada, se cambió por TIMER_C
- **Agregado TIMER_E faltante**: Entre TIMER_D y el TimerConfig final

### STM32F334R8Tx_FLASH.ld
- **Heap aumentado**: De 0x200 (512 bytes) a 0x1000 (4KB) - preparación para FreeRTOS

## Build System

### Herramientas Necesarias (Windows)
1. **GNU Arm Embedded Toolchain 10+**: `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\`
2. **STM32CubeProgrammer**: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`
3. **CMake 4+**: `C:\Program Files\CMake\bin\`
4. **Ninja** (viene con extensión CMake Tools de VS Code)

### Archivos de Configuración
- `build.bat` - Script de compilación directa (funciona inmediatamente)
- `toolchain.cmake` - Configuración de toolchain para CMake
- `CMakeLists.txt` - Build system CMake
- `.vscode/c_cpp_properties.json` - Paths para IntelliSense
- `.vscode/cmake-kits.json` - Kit de compilador GCC ARM
- `.vscode/tasks.json` - Tasks: Build, Flash

### Compilar (2 opciones)
```bash
# Opción 1: build.bat (rápido, sin CMake)
build.bat

# Opción 2: CMake con VSCode
# Ctrl+Shift+P → "CMake: Configure" 
# Ctrl+Shift+B → Build
```

### Flashear
```bash
STM32_Programmer_CLI.exe -c port=SWD -w Controlador-de-temperatura.hex -r 0x08000000 -v
```

## Estado Actual del Firmware

| Recurso | Uso | Límite | % |
|---------|-----|--------|---|
| FLASH (text) | 25,068 bytes | 64KB | 39% |
| RAM (data+bss) | 6,640 bytes | 12KB | 54% |

## Próximos Pasos para RTOS
1. Añadir FreeRTOS (CubeIDE o manualmente)
2. Crear colas para comunicación UART
3. Tarea dedicada para ADC con trigger HRTIM
4. Implementar control PID (funciones stub creadas)

## Comandos UART Soportados
```
S,A    - Start todos los outputs PWM
S,O    - Stop todos los outputs PWM
X,1234 - Set PWM axis X (timers A/B)
Y,567  - Set PWM axis Y (timers C/D)
Z,890  - Set PWM axis Z (timer E)
P,180  - Set fase (phase)
F,150k - Set frecuencia (ej: 150000 = 150kHz)
```

## Notas de Configuración HRTIM
- Clock: 144 MHz (PLL * 9 / 2)
- Periodo Master: 64000 → frecuencia ≈ 2.25 kHz
- Dead time: 200 ticks (prescaler ×8)
- ADC1: 2 canales disparado por HRTIM_TRG1
- Timer mode: SINGLESHOT_RETRIGGERABLE para A-D, CONTINUOUS para E