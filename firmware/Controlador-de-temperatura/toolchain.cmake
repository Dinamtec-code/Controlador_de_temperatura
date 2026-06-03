# Toolchain file for STM32F334 - VS Code compatible
# Set explicit paths based on common installation locations

# Check for ARM_TOOLCHAIN_PATH environment variable first
if(DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(ARM_TOOLCHAIN_PATH "$ENV{ARM_TOOLCHAIN_PATH}" CACHE PATH "ARM toolchain path" FORCE)
elseif(EXISTS "C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi")
    set(ARM_TOOLCHAIN_PATH "C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi" CACHE PATH "ARM toolchain path" FORCE)
else()
    # Common installation paths for GNU Arm toolchain
    set(ARM_TOOLCHAIN_PATH "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10" CACHE PATH "ARM toolchain path")
endif()

set(TOOLCHAIN_PREFIX "arm-none-eabi-")

# Set compilers with full path
set(CMAKE_C_COMPILER "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}gcc.exe")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}g++.exe")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}gcc.exe")
set(CMAKE_OBJCOPY "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}objcopy.exe")
set(CMAKE_OBJDUMP "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}objdump.exe")
set(CMAKE_SIZE_UTIL "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}size.exe")

# Find make program - try common locations
if(EXISTS "C:/msys64/mingw64/bin/mingw32-make.exe")
    set(CMAKE_MAKE_PROGRAM "C:/msys64/mingw64/bin/mingw32-make.exe" CACHE PATH "Make program" FORCE)
elseif(EXISTS "C:/msys64/usr/bin/make.exe")
    set(CMAKE_MAKE_PROGRAM "C:/msys64/usr/bin/make.exe" CACHE PATH "Make program" FORCE)
endif()

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)