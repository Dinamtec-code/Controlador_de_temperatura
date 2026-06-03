#!/bin/bash
# Setup script for Controlador de Temperatura firmware build environment
# Downloads ARM toolchain automatically if not found

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TOOLS_DIR="${SCRIPT_DIR}/tools"

# Check for ARM toolchain
if command -v arm-none-eabi-gcc &> /dev/null; then
    echo "ARM toolchain already in PATH"
    exit 0
fi

# Try common Windows install locations
if [[ -f "C:/Program Files (x86)/GNU Arm Embedded Toolchain/10 2021.10/bin/arm-none-eabi-gcc.exe" ]]; then
    echo "Found ARM toolchain at default Windows location"
    exit 0
fi

# Download ARM toolchain
ARM_TOOLCHAIN_URL="https://developer.arm.com/-/media/Files/downloads/gnu-rm/10-2020q4-major/gcc-arm-none-eabi-10-2020-q4-major-win32.exe"
ARM_TOOLCHAIN_DIR="${TOOLS_DIR}/arm-gnu-toolchain"

mkdir -p "${ARM_TOOLCHAIN_DIR}"

if [[ ! -d "${ARM_TOOLCHAIN_DIR}/bin" ]]; then
    echo "Downloading ARM toolchain..."
    curl -L "${ARM_TOOLCHAIN_URL}" -o "${TOOLS_DIR}/gcc-arm-installer.exe"
    echo "Please run the installer manually, or use --toolchain-path option"
fi

echo "Setup complete. Configure ARM_TOOLCHAIN_PATH if needed."