# Find OpenOCD and flash device
param(
    [string]$HexFile = "Controlador-de-temperatura.hex"
)

$openocdPaths = @(
    "C:/Ac6/SystemWorkbench/plugins/fr.ac6.mcu.externaltools.openocd.win32*/tools/openocd/bin/openocd.exe",
    "${env:ARM_TOOLCHAIN_PATH}/../..//tools/openocd/bin/openocd.exe",
    "C:/tools/openocd/bin/openocd.exe",
    "./tools/openocd/bin/openocd.exe"
)

$openocd = $null
foreach ($pattern in $openocdPaths) {
    $matches = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1
    if ($matches) {
        $openocd = $matches.FullName
        break
    }
}

if (-not $openocd) {
    Write-Error "OpenOCD not found. Install from https://github.com/xpack-dev-tools/openocd-xpack/releases or use AC6 SystemWorkbench"
    exit 1
}

Write-Host "Using OpenOCD: $openocd"

& $openocd -f interface/stlink.cfg -f target/stm32f3x.cfg -c "program $HexFile verify reset exit"