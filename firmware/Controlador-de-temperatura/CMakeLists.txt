cmake_minimum_required(VERSION 3.16)
project(Controlador-de-temperatura C ASM)

set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

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
    Core/Src/services/error_handler.c
)

set(CONTROL_SOURCES
    Core/Src/control/pid_controller.c
    Core/Src/control/arm_pid_init_f32.c
)

set(COMMUNICATION_SOURCES
    Core/Src/communication/comm_sys_api.c
    Core/Src/communication/comm_iface_registry.c
)

set(SYSTEM_SOURCES
    Core/Src/system/config_manager.c
)

set(SCPI_SOURCES
    Core/Src/scpi/scpi_engine.c
    Core/Src/scpi/libscpi/error.c
    Core/Src/scpi/libscpi/expression.c
    Core/Src/scpi/libscpi/fifo_private.h
    Core/Src/scpi/libscpi/fifo.c
    Core/Src/scpi/libscpi/ieee488.c
    Core/Src/scpi/libscpi/lexer_private.h
    Core/Src/scpi/libscpi/lexer.c
    Core/Src/scpi/libscpi/minimal.c
    Core/Src/scpi/libscpi/parser_private.h
    Core/Src/scpi/libscpi/parser.c
    Core/Src/scpi/libscpi/units.c
    Core/Src/scpi/libscpi/utils.c
    Core/Src/scpi/libscpi/utils_private.h
)

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

set(INCLUDE_PATHS
    ${PROJECT_SOURCE_DIR}/Core/Inc
    ${PROJECT_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Inc
    ${PROJECT_SOURCE_DIR}/Drivers/STM32F3xx_HAL_Driver/Inc/Legacy
    ${PROJECT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32F3xx/Include
    ${PROJECT_SOURCE_DIR}/Drivers/CMSIS/Include
)

set(LINKER_SCRIPT ${PROJECT_SOURCE_DIR}/STM32F334R8Tx_FLASH.ld)

add_executable(${PROJECT_NAME}
    ${CORE_SOURCES}
    ${HARDWARE_SOURCES}
    ${TASKS_SOURCES}
    ${SERVICES_SOURCES}
    ${CONTROL_SOURCES}
    ${COMMUNICATION_SOURCES}
    ${SYSTEM_SOURCES}
    ${SCPI_SOURCES}
    ${HAL_SOURCES}
    ${STARTUP_SOURCES}
)

target_include_directories(${PROJECT_NAME} PRIVATE ${INCLUDE_PATHS})

set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_DEPENDS ${LINKER_SCRIPT}
)

target_compile_options(${PROJECT_NAME} PRIVATE -gdwarf-4 -ffunction-sections -fdata-sections -O2 -flto)
target_link_options(${PROJECT_NAME} PRIVATE -flto)
#target_compile_options(${PROJECT_NAME} PRIVATE -gdwarf-4)


target_link_options(${PROJECT_NAME} PRIVATE
    "-Wl,-T${LINKER_SCRIPT}"
    "-Wl,-Map,${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.map"
    "-Wl,--print-memory-usage"
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    -lg_nano
    -lc_nano
    -lgcc
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy "$<TARGET_FILE:${PROJECT_NAME}>" "${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.elf"
    COMMAND "${TOOLCHAIN_ARM_OBJCOPY}" -O ihex "$<TARGET_FILE:${PROJECT_NAME}>" "${PROJECT_SOURCE_DIR}/build/${PROJECT_NAME}.hex"
    COMMAND "${TOOLCHAIN_ARM_SIZE}" "$<TARGET_FILE:${PROJECT_NAME}>"
    COMMENT "Generating elf, hex and printing size info..."
)