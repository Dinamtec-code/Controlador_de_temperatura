@echo off
REM Build script for STM32F334 Temperature Controller (Refactored)
REM ARM GNU Toolchain 15.2.Rel1

call "C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\gccvar.bat"

REM Compiler flags
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -O2 -g3 -ffunction-sections -fdata-sections
set CFLAGS=%CFLAGS% -DSTM32F334x8 -DUSE_HAL_DRIVER -DARM_M4
set CFLAGS=%CFLAGS% -ICore/Inc -IDrivers/STM32F3xx_HAL_Driver/Inc -IDrivers/STM32F3xx_HAL_Driver/Inc/Legacy -IDrivers/CMSIS/Include -IDrivers/CMSIS/Device/ST/STM32F3xx/Include

REM Linker flags  
set LDFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs --specs=nosys.specs
set LDFLAGS=%LDFLAGS% -TSTM32F334R8Tx_FLASH.ld -Wl,-Map=Controlador-de-temperatura.map,--cref -Wl,--gc-sections

REM Source files
set HAL_SRC=Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_cortex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_dma.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_gpio.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_hrtim.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_rcc.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_rcc_ex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_tim.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_tim_ex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_uart.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_uart_ex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_pwr.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_pwr_ex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_flash.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_flash_ex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc_ex.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_i2c.c ^
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_i2c_ex.c

set APP_SRC=Core\Src\main.c ^
Core\Src\stm32f3xx_it.c ^
Core\Src\stm32f3xx_hal_msp.c ^
Core\Src\system_stm32f3xx.c ^
Core\Src\syscalls.c ^
Core\Src\usart.c ^
Core\Src\hrtim.c ^
Core\Src\adc.c ^
Core\Src\gpio.c ^
Core\Src\dma.c ^
Core\Src\hardware\circular_buffer.c ^
Core\Src\hardware\usart_hw.c ^
Core\Src\hardware\adc_hw.c ^
Core\Src\hardware\lcd_hw.c ^
Core\Src\hardware\hrtim_hw.c ^
Core\Src\hardware\i2c_hw.c ^
Core\Src\tasks\scheduler.c ^
Core\Src\tasks\task_comm.c ^
Core\Src\tasks\task_system.c ^
Core\Src\tasks\task_control.c ^
Core\Src\tasks\task_ui.c ^
Core\Src\services\scpi_parser.c ^
Core\Src\services\error_handler.c ^
Core\Src\control\pid_controller.c

echo Compiling...
arm-none-eabi-gcc %CFLAGS% -c %HAL_SRC%
arm-none-eabi-gcc %CFLAGS% -c %APP_SRC%

echo Linking...
arm-none-eabi-gcc %LDFLAGS% *.o startup\startup_stm32f334x8.s -o Controlador-de-temperatura.elf -lm

echo Generating hex...
arm-none-eabi-objcopy -O ihex Controlador-de-temperatura.elf Controlador-de-temperatura.hex

echo Size report:
arm-none-eabi-size Controlador-de-temperatura.elf

echo Build complete!