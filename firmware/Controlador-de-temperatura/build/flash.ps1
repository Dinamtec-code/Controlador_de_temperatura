# Flash STM32 device - STM32CubeProgrammer
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
