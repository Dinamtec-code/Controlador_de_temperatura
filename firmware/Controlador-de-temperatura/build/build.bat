@echo off
REM Build script for STM32F334 temperature controller
REM Build outputs go to this directory (build/)
REM Run this script from any directory - it will work from the script's location

setlocal enabledelayedexpansion

REM Get the script's directory (should be the build folder) and change to it
set SCRIPT_DIR=%~dp0
cd /d "%SCRIPT_DIR%"

REM Get the project root (parent of build/)
set PROJECT_ROOT=%SCRIPT_DIR%..

REM Verify we're in the build directory
if not exist "%PROJECT_ROOT%\Core\Src\main.c" (
    echo ERROR: This script must be run from the build/ directory
    echo Current directory: %CD%
    echo Expected project root: %PROJECT_ROOT%
    exit /b 1
)

REM Clean previous build artifacts
for %%f in (*.o) do del "%%f"
if exist Controlador-de-temperatura.elf del Controlador-de-temperatura.elf
if exist Controlador-de-temperatura.hex del Controlador-de-temperatura.hex
if exist Controlador-de-temperatura.map del Controlador-de-temperatura.map

set "ARM_BIN=C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin"
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -DSTM32F334x8 -DUSE_HAL_DRIVER -Wall -O2 -g3 -ffunction-sections -fdata-sections
set INCLUDES=-I"%PROJECT_ROOT%\Core\Inc" -I"%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Inc" -I"%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Inc\Legacy" -I"%PROJECT_ROOT%\Drivers\CMSIS\Include" -I"%PROJECT_ROOT%\Drivers\CMSIS\Device\ST\STM32F3xx\Include"
set LDFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs --specs=nosys.specs -T"%PROJECT_ROOT%\STM32F334R8Tx_FLASH.ld" -Wl,--gc-sections

echo Compiling application sources...
 for %%f in ("%PROJECT_ROOT%\Core\Src\main.c" "%PROJECT_ROOT%\Core\Src\stm32f3xx_it.c" "%PROJECT_ROOT%\Core\Src\stm32f3xx_hal_msp.c" "%PROJECT_ROOT%\Core\Src\system_stm32f3xx.c" "%PROJECT_ROOT%\Core\Src\syscalls.c" "%PROJECT_ROOT%\Core\Src\usart.c" "%PROJECT_ROOT%\Core\Src\hrtim.c" "%PROJECT_ROOT%\Core\Src\adc.c" "%PROJECT_ROOT%\Core\Src\gpio.c" "%PROJECT_ROOT%\Core\Src\dma.c" "%PROJECT_ROOT%\Core\Src\hardware\usart_hw.c" "%PROJECT_ROOT%\Core\Src\hardware\adc_hw.c" "%PROJECT_ROOT%\Core\Src\hardware\lcd_hw.c" "%PROJECT_ROOT%\Core\Src\hardware\oled_hw.c" "%PROJECT_ROOT%\Core\Src\hardware\hrtim_hw.c" "%PROJECT_ROOT%\Core\Src\hardware\i2c_hw.c" "%PROJECT_ROOT%\Core\Src\tasks\scheduler.c" "%PROJECT_ROOT%\Core\Src\tasks\task_comm.c" "%PROJECT_ROOT%\Core\Src\tasks\task_system.c" "%PROJECT_ROOT%\Core\Src\tasks\task_control.c" "%PROJECT_ROOT%\Core\Src\tasks\task_ui.c" "%PROJECT_ROOT%\Core\Src\services\circular_buffer.c" "%PROJECT_ROOT%\Core\Src\services\scpi_parser.c" "%PROJECT_ROOT%\Core\Src\services\error_handler.c" "%PROJECT_ROOT%\Core\Src\control\pid_controller.c" "%PROJECT_ROOT%\Core\Src\control\arm_pid_init_f32.c" "%PROJECT_ROOT%\Core\Src\communication\comm_buffers.c" "%PROJECT_ROOT%\startup\startup_stm32f334x8.s") do (
     echo Compiling %%f
     "%ARM_BIN%\arm-none-eabi-gcc.exe" -c %CFLAGS% %INCLUDES% %%f -o "%SCRIPT_DIR%%%~nf.o"
 )

echo Compiling HAL sources...
 for %%f in ("%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_cortex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_dma.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_gpio.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_hrtim.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_rcc.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_rcc_ex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_tim.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_tim_ex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_uart.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_uart_ex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_pwr.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_pwr_ex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_flash.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_flash_ex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc_ex.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_i2c.c" "%PROJECT_ROOT%\Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_i2c_ex.c") do (
    echo Compiling %%f
    "%ARM_BIN%\arm-none-eabi-gcc.exe" -c %CFLAGS% %INCLUDES% %%f -o "%SCRIPT_DIR%%%~nf.o"
)

echo Linking...
 dir /b *.o > objects.tmp
 "%ARM_BIN%\arm-none-eabi-gcc.exe" %LDFLAGS% -Wl,-Map=Controlador-de-temperatura.map -Wl,--print-memory-usage -o Controlador-de-temperatura.elf @objects.tmp
 del objects.tmp

echo Generating hex...
 "%ARM_BIN%\arm-none-eabi-objcopy.exe" -O ihex Controlador-de-temperatura.elf Controlador-de-temperatura.hex

echo Build complete.
echo Outputs in Controlador-de-temperatura.elf, Controlador-de-temperatura.hex