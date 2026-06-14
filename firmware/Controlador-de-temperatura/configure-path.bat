@echo off
REM Configure PATH for CMake build tools
set PATH=C:\Program Files\CMake\bin;C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin;%PATH%

echo PATH configured for build. Tools available:
cmake --version 2>nul && echo cmake: OK || echo cmake: NOT FOUND
ninja --version 2>nul && echo ninja: OK || echo ninja: NOT FOUND (optional)
arm-none-eabi-gcc --version 2>nul && echo arm-gcc: OK || echo arm-gcc: NOT FOUND

REM Run build script if exists
if "%1"=="" goto :end
build-cmake.bat %1

:end