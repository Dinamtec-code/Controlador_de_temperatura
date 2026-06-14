# CMake/Ninja Migration Plan

## Project Analysis

### Legacy Build System (build.bat)
- **Target MCU**: STM32F334R8 (ARM Cortex-M4, FPv4-SP-D16 FPU)
- **Compiler**: ARM GNU Toolchain (arm-none-eabi-gcc)
- **Output**: `Controlador-de-temperatura.elf` and `.hex`

### Source Files (EXACTLY from build.bat)

**Core Application (10 files):**
```
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
```

**Hardware Layer (6 files):**
```
Core/Src/hardware/usart_hw.c
Core/Src/hardware/adc_hw.c
Core/Src/hardware/lcd_hw.c
Core/Src/hardware/oled_hw.c
Core/Src/hardware/hrtim_hw.c
Core/Src/hardware/i2c_hw.c
```

**Tasks (5 files):**
```
Core/Src/tasks/scheduler.c
Core/Src/tasks/task_comm.c
Core/Src/tasks/task_system.c
Core/Src/tasks/task_control.c
Core/Src/tasks/task_ui.c
```

**Services (3 files):**
```
Core/Src/services/circular_buffer.c
Core/Src/services/scpi_parser.c
Core/Src/services/error_handler.c
```

**Control (2 files):**
```
Core/Src/control/pid_controller.c
Core/Src/control/arm_pid_init_f32.c
```

**Communication (1 file):**
```
Core/Src/communication/comm_buffers.c
```

**HAL Drivers (18 files) - EXACT LIST from build.bat:**
```
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
```

**Startup (1 file):**
```
startup/startup_stm32f334x8.s
```

### Include Paths (from build.bat)
```
Core/Inc
Drivers/STM32F3xx_HAL_Driver/Inc
Drivers/STM32F3xx_HAL_Driver/Inc/Legacy
Drivers/CMSIS/Include
Drivers/CMSIS/Device/ST/STM32F3xx/Include
```

### Compiler Flags
```
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
-DSTM32F334x8 -DUSE_HAL_DRIVER
-Wall -O2 -g3 -ffunction-sections -fdata-sections
```

### Linker Flags
```
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
-u printf_float --specs=nano.specs --specs=nosys.specs
-Wl,--gc-sections -TSTM32F334R8Tx_FLASH.ld
```

## Issues with Existing CMake Files
1. Toolchain uses unreliable 8.3 short paths
2. CMakeLists.txt doesn't specify toolchain file (must be passed via command line)
3. `stm32f3xx_hal_exti.c` is included but NOT in original build.bat (must be excluded)
4. No explicit objcopy path for post-build hex generation
5. Linker script path may not resolve correctly with build/ directory

## Implementation Plan

### File 1: firmware/Controlador-de-temperatura/cmake/arm-toolchain.cmake

```cmake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(ARM_TOOLCHAIN_DIR "C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin")
set(CMAKE_C_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc.exe")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-g++.exe")
# Startup .s file compiled with gcc -c (not as), so use gcc for ASM
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc.exe")

set(CMAKE_MAKE_PROGRAM "C:/tools/ninja/ninja.exe")

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -DSTM32F334x8 -DUSE_HAL_DRIVER -Wall -O2 -g3 -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -u printf_float --specs=nano.specs --specs=nosys.specs -Wl,--gc-sections")
```

### File 2: firmware/Controlador-de-temperatura/CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.16)
project(Controlador-de-temperatura C ASM)

# Build configuration
set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")

# Output directory (in-project build directory for flash script compatibility)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# Source files
set(CORE_SOURCES
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
)

set(HARDWARE_SOURCES
    Core/Src/hardware/usart_hw.c
    Core/Src/hardware/adc_hw.c
    Core/Src/hardware/lcd_hw.c
    Core/Src/hardware/oled_hw.c
    Core/Src/hardware/hrtim_hw.c
    Core/Src/hardware/i2c_hw.c
)

set(TASKS_SOURCES
    Core/Src/tasks/scheduler.c
    Core/Src/tasks/task_comm.c
    Core/Src/tasks/task_system.c
    Core/Src/tasks/task_control.c
    Core/Src/tasks/task_ui.c
)

set(SERVICES_SOURCES
    Core/Src/services/circular_buffer.c
    Core/Src/services/scpi_parser.c
    Core/Src/services/error_handler.c
)

set(CONTROL_SOURCES
    Core/Src/control/pid_controller.c
    Core/Src/control/arm_pid_init_f32.c
)

set(COMMUNICATION_SOURCES
    Core/Src/communication/comm_buffers.c
)

# HAL Drivers (EXACTLY 18 files from build.bat - NO hal_exti.c)
set(HAL_SOURCES
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

# Include directories
set(INCLUDE_PATHS
    ${PROJECT_SOURCE_DIR}/Core/Inc
    ${PROJECT_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Inc
    ${PROJECT_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy
    ${PROJECT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F3xx/Include
    ${PROJECT_SOURCE_DIR}/Drivers/CMSIS/Include
)

# Linker script
set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/STM32F334R8Tx_FLASH.ld)

# Executable
add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${HARDWARE_SOURCES}
    ${TASKS_SOURCES}
    ${SERVICES_SOURCES}
    ${CONTROL_SOURCES}
    ${COMMUNICATION_SOURCES}
    ${HAL_SOURCES}
    ${STARTUP_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_PATHS})

# Linker script specification
set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
)

target_link_options(${PROJECT_NAME} PRIVATE
    "-T${LINKER_SCRIPT}"
    "-Map=${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.map"
    "--print-memory-usage"
)

# Post-build: generate hex file and show size
find_program(ARM_OBJCOPY NAMES arm-none-eabi-objcopy REQUIRED)
find_program(ARM_SIZE NAMES arm-none-eabi-size REQUIRED)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${ARM_OBJCOPY} -O ihex "$<TARGET_FILE:${PROJECT_NAME}>" "${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.hex"
    COMMAND ${ARM_SIZE} "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Generating hex and printing size info..."
)
```

### File 3: firmware/Controlador-de-temperatura/configure-cmake.bat (UPDATE)

```batch
@echo off
REM Configure PATH and run CMake with Ninja generator
set "PATH=C:\Program Files\CMake\bin;C:\tools\ninja;C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin;%PATH%"

echo Configuring project with CMake...
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Configuration complete. Run 'cmake --build build' to compile.
```

## Flash Script Compatibility

The existing `build/flash.ps1` expects `Controlador-de-temperatura.hex` in the `build/` directory. The new CMakeLists.txt outputs all build artifacts (elf, hex, map) to `build/` via:
- `CMAKE_RUNTIME_OUTPUT_DIRECTORY` for the elf
- Post-build command outputting hex to `${PROJECT_SOURCE_DIR}/build/`

## Build Commands (Post-Migration)

Configure:
```cmd
cmake -B build -S . -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/arm-toolchain.cmake
```

Build:
```cmd
cmake --build build
```

Flash:
```cmd
build\flash.bat
```