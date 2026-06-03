cmake_minimum_required(VERSION 3.20)

# Project settings
set(PROJECT_NAME "Controlador-de-temperatura")
project(${PROJECT_NAME} C ASM)

# Target settings for STM32F334R8 (Cortex-M4, FPU)
set(TARGET_CPU "cortex-m4")
set(TARGET_FPU "-mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# Toolchain prefix
set(TOOLCHAIN_PREFIX "arm-none-eabi-")

# Include directories
set(INCLUDE_DIRS
    Core/Inc
    Drivers/STM32F3xx_HAL_Driver/Inc
    Drivers/STM32F3xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Include
    Drivers/CMSIS/Device/ST/STM32F3xx/Include
)

# Compiler flags
set(COMMON_FLAGS "-mcpu=${TARGET_CPU} -mthumb ${TARGET_FPU} -Wall -O2 -g3 -ffunction-sections -fdata-sections")
set(CMAKE_C_FLAGS "${COMMON_FLAGS}" CACHE STRING "" FORCE)
set(CMAKE_ASM_FLAGS "${COMMON_FLAGS}" CACHE STRING "" FORCE)

# Linker flags
set(LINKER_FLAGS "-mcpu=${TARGET_CPU} -mthumb ${TARGET_FPU} --specs=nano.specs --specs=nosys.specs -T${CMAKE_SOURCE_DIR}/STM32F334R8Tx_FLASH.ld -Wl,-Map=${PROJECT_NAME}.map,--cref -Wl,--gc-sections -Wl,--print-memory-usage")

# Definitions
set(COMMON_DEFINES 
    "-DSTM32F334x8"
    "-DUSE_HAL_DRIVER"
    "-DARM_M4"
)

# HAL Driver source files
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
)

# HAL I2C source
set(HAL_I2C_SOURCE
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c.c
    Drivers/STM32F3xx_HAL_Driver/Src/stm32f3xx_hal_i2c_ex.c
)

# Application source files (refactored architecture)
set(APP_SOURCES
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
    # Hardware abstraction layer
    Core/Src/hardware/circular_buffer.c
    Core/Src/hardware/usart_hw.c
    Core/Src/hardware/adc_hw.c
    Core/Src/hardware/lcd_hw.c
    Core/Src/hardware/hrtim_hw.c
    Core/Src/hardware/i2c_hw.c
    # Tasks
    Core/Src/tasks/scheduler.c
    Core/Src/tasks/task_comm.c
    Core/Src/tasks/task_system.c
    Core/Src/tasks/task_control.c
    Core/Src/tasks/task_ui.c
    # Services
    Core/Src/services/scpi_parser.c
    Core/Src/services/error_handler.c
    # Control
    Core/Src/control/pid_controller.c
)

set(STARTUP_SOURCE ${CMAKE_SOURCE_DIR}/startup/startup_stm32f334x8.s)

# Build executable
add_executable(${PROJECT_NAME}.elf 
    ${APP_SOURCES}
    ${HAL_SOURCES}
    ${HAL_I2C_SOURCE}
    ${STARTUP_SOURCE}
)

# Include directories
target_include_directories(${PROJECT_NAME}.elf PRIVATE ${INCLUDE_DIRS})

# Compiler definitions
target_compile_definitions(${PROJECT_NAME}.elf PRIVATE 
    ${COMMON_DEFINES}
)

# Linker options
set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "" FORCE)

# Custom targets for post-build
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE_UTIL} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating hex and binary files..."
)

# Flash target using OpenOCD (falls back to STM32CubeProgrammer)
set(OPENOCD_EXE "C:/Ac6/SystemWorkbench/plugins/fr.ac6.mcu.externaltools.openocd.win32_1.23.0.201904120827/tools/openocd/bin/openocd.exe")
if(EXISTS "${OPENOCD_EXE}")
    add_custom_target(flash
        COMMAND "${OPENOCD_EXE}" -f interface/stlink.cfg -f target/stm32f3x.cfg -c "program ${PROJECT_NAME}.hex verify reset exit"
        DEPENDS ${PROJECT_NAME}.elf
        COMMENT "Flashing device with OpenOCD..."
    )
else()
    set(STLINK_CLI "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe")
    if(EXISTS "${STLINK_CLI}")
        add_custom_target(flash
            COMMAND "${STLINK_CLI}" -c port=SWD -w ${PROJECT_NAME}.hex -r 0x08000000 -v
            COMMAND "${STLINK_CLI}" -c port=SWD -hardrst
            DEPENDS ${PROJECT_NAME}.elf
            COMMENT "Flashing device..."
        )
    endif()
endif()

# Clean extended target
add_custom_target(clean_all
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/cmake_clean.cmake
    COMMAND ${CMAKE_COMMAND} -E remove -f *.hex *.bin *.map
    COMMENT "Cleaning all generated files..."
)