@echo off
REM Flash STM32 device using STM32CubeProgrammer
REM Looks for hex file in build/ directory

setlocal

set "PROJECT_ROOT=%~dp0"
set "HEX_FILE=Controlador-de-temperatura.hex"
set "HEX_PATH=%PROJECT_ROOT%build\%HEX_FILE%"

if not exist "%HEX_PATH%" (
    echo ERROR: Hex file not found: %HEX_PATH%
    echo Run cmake --build build first
    exit /b 1
)

set "STM32_CUBE_PROG=C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"

if not exist "%STM32_CUBE_PROG%" (
    set "STM32_CUBE_PROG=%PROGRAMFILES(X86)%\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
)

if not exist "%STM32_CUBE_PROG%" (
    echo ERROR: STM32CubeProgrammer not found
    echo Install from https://www.st.com/en/development-tools/stm32cubeprogrammer.html
    exit /b 1
)

echo Using STM32CubeProgrammer: %STM32_CUBE_PROG%
echo Flashing: %HEX_PATH%
"%STM32_CUBE_PROG%" -c port=SWD -w "%HEX_PATH%" -v
if %ERRORLEVEL% EQU 0 (
    "%STM32_CUBE_PROG%" -c port=SWD -hardrst
    echo Flash complete.
) else (
    echo Flash failed.
    exit /b 1
)