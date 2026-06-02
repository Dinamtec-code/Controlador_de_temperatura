# Toolchain file for STM32F334 - VS Code compatible
# Set explicit paths based on common installation locations

# Common installation paths for GNU Arm toolchain
set(ARM_TOOLCHAIN_PATH "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10" CACHE PATH "ARM toolchain path")

set(TOOLCHAIN_PREFIX "arm-none-eabi-")

# Set compilers with full path
set(CMAKE_C_COMPILER "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}gcc.exe")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}g++.exe")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}gcc.exe")
set(CMAKE_OBJCOPY "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}objcopy.exe")
set(CMAKE_OBJDUMP "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}objdump.exe")
set(CMAKE_SIZE_UTIL "${ARM_TOOLCHAIN_PATH}/bin/${TOOLCHAIN_PREFIX}size.exe")

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)