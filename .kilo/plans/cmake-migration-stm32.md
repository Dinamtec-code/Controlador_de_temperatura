# Plan de Migración a CMake para Controlador de Temperatura (STM32F334)

## Análisis del Proyecto Actual

## Toolchain Detectado

- **CMake**: `C:\Program Files\CMake\bin` (instalado)
- **ARM-GCC**: `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin` (usar esta versión más nueva)
- **Target**: STM32F334R8 (Cortex-M4, FPU fpv4-sp-d16, hard float)
- **CFLAGS**: `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32F334x8 -DUSE_HAL_DRIVER -Wall -O2 -g3 -ffunction-sections -fdata-sections`
- **LDFLAGS**: `-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -u printf_float --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections`
- **Salidas**: `.elf`, `.hex`, `.map`

### Estructura del Proyecto
```
Controlador-de-temperatura/
├── Core/
│   ├── Inc/           # Headers del proyecto (tasks, services, control, hardware, communication)
│   └── Src/           # Sources del proyecto
├── Drivers/           # CMSIS + HAL Driver
└── startup/           # startup_stm32f334x8.s
```

### Sources a Compilar (del build.bat)
**Core Application (21 archivos)**:
- main.c, stm32f3xx_it.c, stm32f3xx_hal_msp.c, system_stm32f3xx.c, syscalls.c, usart.c, hrtim.c, adc.c, gpio.c, dma.c
- hardware/*.c (usart_hw, adc_hw, lcd_hw, oled_hw, hrtim_hw, i2c_hw)
- tasks/*.c (scheduler, task_comm, task_system, task_control, task_ui)
- services/*.c (circular_buffer, scpi_parser, error_handler)
- control/*.c (pid_controller, arm_pid_init_f32)

**HAL Driver (22 archivos)**: Drivers/STM32F3xx_HAL_Driver/Src/*.c

**Startup**: startup_stm32f334x8.s

## Pasos para Migración

### Paso 1: Verificar/Instalar Herramientas Necesarias

**Preguntas para el usuario:**
1. ¿Quieres instalar herramientas adicionales o usar las existentes? (STM32CubeIDE incluye CMake integrado)

**Herramientas requeridas:**
- ✅ **CMake** (>= 3.16) - verificar si está instalado
- ✅ **Ninja** (opcional, alternativa a Make) - verificar si está instalado
- ✅ **ARM-GCC** - ya instalado en ruta específica

**Si faltan herramientas:**
- **Opción A (recomendado)**: STM32CubeIDE (incluye ARM-GCC + CMake integrado)
- **Opción B**: Instalar ARM-GCC standalone + CMake + Ninja por separado
  - ARM-GCC: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads
  - CMake: https://cmake.org/download/
  - Ninja: https://ninja-build.org/

### Paso 2: Verificar Toolchain ARM-GCC

Verificar que `C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin` existe y contiene:
- `arm-none-eabi-gcc.exe`
- `arm-none-eabi-g++.exe`
- `arm-none-eabi-objcopy.exe`
- `arm-none-eabi-size.exe`

### Paso 3: Crear CMakeLists.txt

Crear `firmware/Controlador-de-temperatura/CMakeLists.txt` con:

```cmake
cmake_minimum_required(VERSION 3.16)
project(Controlador-de-temperatura C ASM)

# Toolchain configuration
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Cross-compiler
set(ARM_TOOLCHAIN "C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin")
set(ARM_CC "${ARM_TOOLCHAIN}/arm-none-eabi-gcc")
set(ARM_OBJCOPY "${ARM_TOOLCHAIN}/arm-none-eabi-objcopy")
set(ARM_SIZE "${ARM_TOOLCHAIN}/arm-none-eabi-size")

set(CMAKE_C_COMPILER ${ARM_CC})
set(CMAKE_CXX_COMPILER ${ARM_CC})
set(CMAKE_ASM_COMPILER ${ARM_CC})

# CPU flags
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# Preprocessor defines
set(DEFINES 
    -DSTM32F334x8 
    -DUSE_HAL_DRIVER
)

# Compiler flags
set(CMAKE_C_FLAGS "${CPU_FLAGS} ${DEFINES} -Wall -O2 -g3 -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS_DEBUG "-O0 -g3")
set(CMAKE_C_FLAGS_RELEASE "-O2")

# Linker flags
set(LINKER_FLAGS "${CPU_FLAGS} -specs=nosys.specs -specs=nano.specs -u printf_float -Wl,--gc-sections")

# Include paths
set(INCLUDE_PATHS
    "${PROJECT_SOURCE_DIR}/Core/Inc"
    "${PROJECT_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Inc"
    "${PROJECT_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy"
    "${PROJECT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F3xx/Include"
    "${PROJECT_SOURCE_DIR}/Drivers/CMSIS/Include"
)

# Source files
set(CORE_SOURCES
    # Core/Src/*.c
    Core/Src/main.c
    Core/Src/stm32f3xx_it.c
    Core/Src/stm32f3xx_hal_msp.c
    Core/Src/system_stm32f3xx.c
    Core/Src/syscalls.c
    Core/Src/usart.c
    Core/Src/hrtim.c
    Core/Src/adc.c
    Core/Src/gpio.c
    Core/Src/dma.c
    # hardware
    Core/Src/hardware/usart_hw.c
    Core/Src/hardware/adc_hw.c
    Core/Src/hardware/lcd_hw.c
    Core/Src/hardware/oled_hw.c
    Core/Src/hardware/hrtim_hw.c
    Core/Src/hardware/i2c_hw.c
    # tasks
    Core/Src/tasks/scheduler.c
    Core/Src/tasks/task_comm.c
    Core/Src/tasks/task_system.c
    Core/Src/tasks/task_control.c
    Core/Src/tasks/task_ui.c
    # services
    Core/Src/services/circular_buffer.c
    Core/Src/services/scpi_parser.c
    Core/Src/services/error_handler.c
    # control
    Core/Src/control/pid_controller.c
    Core/Src/control/arm_pid_init_f32.c
    # communication
    Core/Src/communication/comm_buffers.c
)

set(HAL_SOURCES
    # HAL Driver sources
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_cortex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_dma.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_gpio.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_hrtim.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_rcc_ex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_tim_ex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_uart.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_uart_ex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_pwr_ex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_flash_ex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_adc.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_adc_ex.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c_ex.c
)

set(STARTUP_SOURCES
    startup/startup_stm32f334x8.s
)

# Executable
add_executable(${PROJECT_NAME}.elf
    ${CORE_SOURCES}
    ${HAL_SOURCES}
    ${STARTUP_SOURCES}
)

target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INCLUDE_PATHS})

# Linker script
set(LINKER_SCRIPT "${PROJECT_SOURCE_DIR}/STM32F334R8Tx_FLASH.ld")
set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS} -T${LINKER_SCRIPT}")

# Post-build: generate hex and show size
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${ARM_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.hex
    COMMAND ${ARM_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)

# Clean target
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/*.o
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_SOURCE_DIR}/build/*.elf
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_SOURCE_DIR}/build/*.hex
    COMMAND ${CMAKE_COMMAND} -E remove -f ${PROJECT_SOURCE_DIR}/build/*.map
)
```

### Paso 4: Configurar Generador Ninja (Opcional)

Si se instala Ninja, crear `build-ninja.bat`:

```batch
@echo off
cd /d "%~dp0"
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Paso 5: Limpiar Herramientas Conflictivas

**Herramientas a revisar:**
1. El script `build.bat` actual en `build/build.bat` - mantener como respaldo o eliminar
2. Archivos `.cproject`, `.project`, `.mxproject` - opcional: eliminar si no se usa STM32CubeIDE
3. Verificar que no haya otras instancias de ARM-GCC en PATH que puedan interferir

**Recomendación**: Agregar `C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin` al PATH del sistema o usar CMake con rutas absolutas.

### Paso 6: Verificar FreeRTOS (Dependencia)

El `.ioc` menciona FreeRTOS pero el `.gitignore` lo excluye. El `build.bat` actual **no lo incluye**. Verificar si el proyecto necesita FreeRTOS:
- Revisa si hay llamadas a FreeRTOS en el código (por ejemplo `osThreadNew`, `osMessageQueue...`)
- Si no se usa, el build actual es correcto sin FreeRTOS

### Paso 7: Comandos de Build Finales

```bash
# Para build con CMake (generador predeterminado)
cmake -B build
cmake --build build

# Para build con Ninja
cmake -B build -G Ninja
cmake --build build

# Para limpiar
cmake --build build --target clean
# o manualmente: del build\*.o build\*.elf build\*.hex build\*.map
```

## Archivos Creados

| Archivo | Descripción |
|---------|-------------|
| `CMakeLists.txt` | ✅ Configuración principal de CMake |
| `cmake/arm-toolchain.cmake` | ✅ Archivo de toolchain separado |
| `build-cmake.bat` | ✅ Script de build (agrega PATH automáticamente) |
| `check-tools.bat` | ✅ Verificador de herramientas instaladas |
| `.vscode/settings.json` | ✅ Configuración de VSCode para CMake Tools |
| `.gitignore` | ✅ Actualizado para archivos CMake |

## Notas Importantes

1. **Mantiene la misma versión de ARM-GCC** - usa la instalada actualmente
2. **Compatible con Linux/macOS** - CMake detectará automáticamente el toolchain arm-none-eabi-gcc desde PATH
3. **Salida idéntica** - produce los mismos archivos `.elf`, `.hex`, `.map`
4. **FreeRTOS no incluido** - el build.bat original tampoco lo incluye, si se necesita será agregado después

## Verificación Post-Migración

1. Comparar binario generado con el del build.bat
2. Verificar tamaño y símbolos con `arm-none-eabi-size`
3. Verificar que el mapa de memoria coincida

---

## Respuestas del Usuario

- ✅ **Usar toolchain actual** (ARM-GCC en `C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin`)
- ✅ **Instalar Ninja** para builds más rápidos

## Instalación de Ninja (Windows)

1. Descargar ninja desde: https://github.com/ninja-build/ninja/releases/latest
2. Descoger `ninja-win.zip` 
3. Extraer `ninja.exe` a una carpeta (ej: `C:\tools\ninja`)
4. Agregar `C:\tools\ninja` al PATH del sistema
5. Verificar: abrir nueva terminal y ejecutar `ninja --version`

## Instalación de CMake (Windows) - YA INSTALADO

- CMake está en: `C:\Program Files\CMake\bin`
- Agregar esta ruta al PATH del sistema o usar `configure-path.bat`

## Agregar al PATH de Windows

1. Abrir "Editar variables de entorno del sistema"
2. Editar la variable `Path` (PATH) en "Variables de sistema"
3. Agregar nuevos valores:
   - `C:\Program Files\CMake\bin`
   - `C:\tools\ninja` (si instalaste ninja ahí)
   - `C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin`
4. Reiniciar terminal/VSCode

O usar `build-cmake.bat` que configura el PATH temporalmente automáticamente.

## Uso

```cmd
# Build con CMake (desde el directorio del proyecto)
cd firmware\Controlador-de-temperatura
build-cmake.bat

# O manualmente:
cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
cmake --build build

# Comparar outputs
fc /b build\Controlador-de-temperatura.hex ..\build\Controlador-de-temperatura.hex
```