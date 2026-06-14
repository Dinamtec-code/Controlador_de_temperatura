@echo off
REM CMake build script for Controlador-de-temperatura
REM Uses Ninja generator with ARM toolchain

set "PATH=C:\Program Files\CMake\bin;C:\tools\ninja;C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin;%PATH%"

echo Configuring project with CMake...
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="cmake/arm-toolchain.cmake"
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Building project...
cmake --build build
if %ERRORLEVEL% NEQ 0 (
    echo Build failed!
    exit /b 1
)

echo Build complete. Output: build\Controlador-de-temperatura.elf
echo Hex file: build\Controlador-de-temperatura.hex