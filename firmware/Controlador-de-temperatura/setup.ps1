# Setup script for Controlador de Temperatura firmware
# Downloads OpenOCD portable if not found in system

param(
    [string]$ToolsDir = "$(Split-Path -Parent $MyInvocation.MyCommand.Path)\tools"
)

$ErrorActionPreference = "Stop"

function Find-OpenOCD {
    $paths = @(
        "C:\Ac6\SystemWorkbench\plugins\fr.ac6.mcu.externaltools.openocd.win32*\tools\openocd\bin\openocd.exe",
        "${env:ARM_TOOLCHAIN_PATH}\..\..\tools\openocd\bin\openocd.exe",
        "C:\tools\openocd\bin\openocd.exe",
        "${ToolsDir}\openocd-xpack\bin\openocd.exe"
    )
    
    foreach ($p in $paths) {
        $match = Get-ChildItem -Path $p -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1
        if ($match) { return $match.FullName }
    }
    return $null
}

function Find-ARM-GCC {
    $paths = @(
        "arm-none-eabi-gcc",
        "C:\Program Files (x86)\GNU Arm Embedded Toolchain\10 2021.10\bin\arm-none-eabi-gcc.exe",
        "C:\Program Files (x86)\Arm\GNU Toolchain mingw-w64-i686-arm-none-eabi\bin\arm-none-eabi-gcc.exe"
    )
    
    foreach ($p in $paths) {
        if (Get-Command $p -ErrorAction SilentlyContinue) { return $p }
    }
    return $null
}

Write-Host "Checking tools..."

# Check ARM GCC
$armGcc = Find-ARM-GCC
if (-not $armGcc) {
    Write-Warning "ARM GCC not found. Install from: https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm"
    exit 1
}
Write-Host "Found ARM GCC: $armGcc"

# Check OpenOCD
$openocd = Find-OpenOCD
if (-not $openocd) {
    Write-Host "OpenOCD not found, downloading portable version..."
    
    $openocdUrl = "https://github.com/xpack-dev-tools/openocd-xpack/releases/download/v0.12.0-2/openocd-0.12.0-2-win32-x64.tar.gz"
    $toolsDir = New-Item -ItemType Directory -Path $ToolsDir -Force
    $archive = Join-Path $toolsDir "openocd.tar.gz"
    
    # Download
    Invoke-WebRequest -Uri $openocdUrl -OutFile $archive
    
    # Extract
    tar -xzf $archive -C $toolsDir
    Remove-Item $archive
    
    $openocd = Join-Path $toolsDir "openocd-0.12.0-2-win32-x64/bin/openocd.exe"
    
    if (-not (Test-Path $openocd)) {
        Write-Error "Failed to install OpenOCD"
        exit 1
    }
    Write-Host "OpenOCD downloaded to: $openocd"
} else {
    Write-Host "Found OpenOCD: $openocd"
}

# Add to PATH for this session
$env:Path += ";$(Split-Path $openocd)"

Write-Host "Setup complete. Build with: build.bat"
Write-Host "Flash with: .\flash.ps1"

# Download FreeRTOS if needed
$rtosDir = Join-Path $ToolsDir "FreeRTOS"
if (-not (Test-Path "$rtosDir/include/tasks.h")) {
    Write-Host "Downloading FreeRTOS portable..."
    $freertosUrl = "https://raw.githubusercontent.com/FreeRTOS/FreeRTOS/main/FreeRTOS/Source/include/tasks.h"
    # For now, just note the requirement
    Write-Warning "FreeRTOS requires manual installation or download. See setup.ps1 for details."
}