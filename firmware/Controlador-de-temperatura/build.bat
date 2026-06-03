@echo off
REM Build script for STM32F334 Multi Phase Buck Project
REM Assumes GNU Arm toolchain is installed at default location

set TOOLCHAIN="C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin"
set CC=%TOOLCHAIN%\arm-none-eabi-gcc.exe
set OBJCOPY=%TOOLCHAIN%\arm-none-eabi-objcopy.exe
set SIZE=%TOOLCHAIN%\arm-none-eabi-size.exe

REM Compiler flags
set CFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard -Wall -O2 -g3 -ffunction-sections -fdata-sections
set CFLAGS=%CFLAGS% -DSTM32F334x8 -DUSE_HAL_DRIVER -DARM_M4
set CFLAGS=%CFLAGS% -ICore/Inc -IDrivers/STM32F3xx_HAL_Driver/Inc -IDrivers/STM32F3xx_HAL_Driver/Inc/Legacy -IDrivers/CMSIS/Include -IDrivers/CMSIS/Device/ST/STM32F3xx/Include

REM Linker flags  
set LDFLAGS=-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard --specs=nano.specs --specs=nosys.specs
set LDFLAGS=%LDFLAGS% -TSTM32F334R8Tx_FLASH.ld -Wl,-Map=Multi_phase_buck.map,--cref -Wl,--gc-sections

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
Drivers\STM32F3xx_HAL_Driver\Src\stm32f3xx_hal_adc_ex.c

set APP_SRC=Core\Src\main.c ^
Core\Src\stm32f3xx_it.c ^
Core\Src\stm32f3xx_hal_msp.c ^
Core\Src\system_stm32f3xx.c ^
Core\Src\syscalls.c ^
Core\Src\usart.c ^
Core\Src\hrtim.c ^
Core\Src\adc.c ^
Core\Src\gpio.c ^
Core\Src\dma.c

echo Compiling...
%CC% %CFLAGS% -c %HAL_SRC%
%CC% %CFLAGS% -c %APP_SRC%

echo Linking...
%CC% %LDFLAGS% *.o startup\startup_stm32f334x8.s -o Multi_phase_buck.elf

echo Generating hex...
%OBJCOPY% -O ihex Multi_phase_buck.elf Multi_phase_buck.hex

echo Size report:
%SIZE% Multi_phase_buck.elf

echo Build complete!