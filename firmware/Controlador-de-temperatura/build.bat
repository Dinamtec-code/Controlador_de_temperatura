@echo off
REM Build script for STM32F334 temperature controller
REM Build outputs go to build/ directory

setlocal enabledelayedexpansion

set ARM_BIN="C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin"
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32F334x8 -DUSE_HAL_DRIVER -Wall -O2 -g3 -ffunction-sections -fdata-sections
set INCLUDES=-ICore\Inc -IDrivers\STM32F3xx_HAL_Driver\Inc -IDrivers\STM32F3xx_HAL_Driver\Inc\Legacy -IDrivers\CMSIS\Include -IDrivers\CMSIS\Device\ST\STM32F3xx\Include
set LDFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs --specs=nosys.specs -TSTM32F334R8Tx_FLASH.ld -Wl,--gc-sections

REM Create build directory
if not exist build mkdir build

echo Compiling application sources...
for %%f in (Core\Src\main.c Core\Src\stm32f3xx_it.c Core\Src\stm32f3xx_hal_msp.c Core\Src\system_stm32f3xx.c Core\Src\syscalls.c Core\Src\usart.c Core\Src\hrtim.c Core\Src\adc.c Core\Src\gpio.c Core\Src\dma.c Core\Src\hardware\circular_buffer.c Core\Src\hardware\usart_hw.c Core\Src\hardware\adc_hw.c Core\Src\hardware\lcd_hw.c Core\Src\hardware\oled_hw.c Core\Src\hardware\hrtim_hw.c Core\Src\hardware\i2c_hw.c Core\Src\tasks\scheduler.c Core\Src\tasks\task_comm.c Core\Src\tasks\task_system.c Core\Src\tasks\task_control.c Core\Src\tasks\task_ui.c Core\Src\services\scpi_parser.c Core\Src\services\error_handler.c Core\Src\control\pid_controller.c Core\Src\control\arm_pid_init_f32.c startup\startup_stm32f334x8.s) do (
    echo Compiling %%f
    %ARM_BIN%\arm-none-eabi-gcc.exe -c %CFLAGS% %INCLUDES% %%f -o build\%%~nf.o
)

echo Compiling HAL sources...
for %%f in (Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_cortex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_dma.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_gpio.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_hrtim.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_rcc.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_rcc_ex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_tim.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_tim_ex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_uart.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_uart_ex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_pwr.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_pwr_ex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_flash.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_flash_ex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc_ex.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_i2c.c Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_i2c_ex.c) do (
    echo Compiling %%f
    %ARM_BIN%\arm-none-eabi-gcc.exe -c %CFLAGS% %INCLUDES% %%f -o build\%%~nf.o
)

echo Linking...
%ARM_BIN%\arm-none-eabi-gcc.exe %LDFLAGS% -Wl,-Map=build\Controlador-de-temperatura.map -Wl,--print-memory-usage -o build\Controlador-de-temperatura.elf build\*.o

echo Generating hex...
%ARM_BIN%\arm-none-eabi-objcopy.exe -O ihex build\Controlador-de-temperatura.elf Controlador-de-temperatura.hex

echo Build complete.