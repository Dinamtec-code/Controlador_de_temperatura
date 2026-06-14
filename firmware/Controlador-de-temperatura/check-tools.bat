@echo off
REM Verify build tools are available

echo Checking build tools...
echo.

echo [1] CMake:
cmake --version >nul 2>&1 && echo    OK - found || echo    MISSING - install from https://cmake.org/download/

echo [2] Ninja (optional):
ninja --version >nul 2>&1 && echo    OK - found || echo    MISSING - download from https://github.com/ninja-build/ninja/releases

echo [3] ARM-GCC:
arm-none-eabi-gcc --version >nul 2>&1 && echo    OK - found || (
    echo    Checking alternate location...
    "C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin" --version >nul 2>&1 && echo    OK - found at alternate path || echo    MISSING - install ARM-GCC
)

echo.
echo Toolchain paths detected:
echo    CMake:    C:\Program Files\CMake\bin
echo    Ninja:    c:\tools\ninja
echo    ARM-GCC:  C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin