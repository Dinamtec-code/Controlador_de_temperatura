set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

set(ARM_TOOLCHAIN_DIR "C:/Program Files (x86)/Arm/GNU Toolchain mingw-w64-i686-arm-none-eabi/bin")
set(ARM_TOOLCHAIN_DIR "${ARM_TOOLCHAIN_DIR}" CACHE PATH "ARM toolchain directory")

set(CMAKE_C_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc.exe" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-g++.exe" CACHE FILEPATH "C++ compiler")
set(CMAKE_ASM_COMPILER "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-gcc.exe" CACHE FILEPATH "ASM compiler")

set(CMAKE_MAKE_PROGRAM "C:/tools/ninja/ninja.exe" CACHE FILEPATH "Make program")

set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -DSTM32F334x8 -DUSE_HAL_DRIVER -Wall -O2 -gdwarf-4 -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -gdwarf-4")
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -u printf_float -Wl,--gc-sections --specs=nano.specs --specs=nosys.specs -nostdlib")
set(CMAKE_C_IMPLICIT_LINK_LIBRARIES "" CACHE INTERNAL "")

set(CMAKE_C_COMPILER_WORKS TRUE CACHE INTERNAL "")
set(CMAKE_CXX_COMPILER_WORKS TRUE CACHE INTERNAL "")
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

set(TOOLCHAIN_ARM_OBJCOPY "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-objcopy.exe" CACHE FILEPATH "objcopy")
set(TOOLCHAIN_ARM_SIZE "${ARM_TOOLCHAIN_DIR}/arm-none-eabi-size.exe" CACHE FILEPATH "size")