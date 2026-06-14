@echo off
REM Configure PATH and run CMake with Ninja generator
set "PATH=C:\Program Files\CMake\bin;C:\tools\ninja;C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin;%PATH%"

echo PATH configured:
echo   CMake: C:\Program Files\CMake\bin
echo   Ninja: C:\tools\ninja
echo   ARM-GCC: C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin

echo Configuring project with CMake...
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_TOOLCHAIN_FILE="cmake/arm-toolchain.cmake"
if %ERRORLEVEL% NEQ 0 (
    echo CMake configuration failed!
    exit /b 1
)

echo Configuration complete. Run 'cmake --build build' to compile.