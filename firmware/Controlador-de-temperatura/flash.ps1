# Flash STM32 device - tries STM32CubeProgrammer first, then OpenOCD
param(
    [string]$HexFile = "Controlador-de-temperatura.hex"
)

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$HexFullPath = Join-Path $ScriptDir $HexFile

# Try STM32CubeProgrammer first (official ST tool, more reliable)
$stm32ProgPaths = @(
    "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
    "${env:PROGRAMFILES}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
    "${env:PROGRAMFILES(X86)}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
)

foreach ($path in $stm32ProgPaths) {
    if (Test-Path $path) {
        Write-Host "Using STM32CubeProgrammer: $path"
        & $path -c port=SWD -w $HexFullPath -v
        if ($LASTEXITCODE -eq 0) {
            & $path -c port=SWD -hardrst
            exit 0
        }
        exit 1
    }
}

# Fallback to OpenOCD (try AC6, then portable)
$openocdPaths = @(
    "C:/Ac6/SystemWorkbench/plugins/fr.ac6.mcu.externaltools.openocd.win32*/tools/openocd/bin/openocd.exe",
    "C:/Program Files (x86)/Arm/GNU Toolchain*/tools/openocd/bin/openocd.exe",
    "./tools/openocd/bin/openocd.exe"
)

$openocd = $null
foreach ($pattern in $openocdPaths) {
    $match = Get-ChildItem -Path $pattern -ErrorAction SilentlyContinue | Sort-Object LastWriteTime | Select-Object -Last 1
    if ($match) {
        $openocd = $match.FullName
        break
    }
}

if (-not $openocd) {
    Write-Error "No programmer found. Install STM32CubeProgrammer or OpenOCD."
    exit 1
}

Write-Host "Using OpenOCD (fallback): $openocd"
& $openocd -f interface/stlink.cfg -f target/stm32f3x.cfg -c "program $HexFullPath verify reset exit"