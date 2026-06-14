@echo off
REM Configure PATH and run CMake config + build
set PATH=C:\Program Files\CMake\bin;C:\tools\ninja;C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin;%PATH%

cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build