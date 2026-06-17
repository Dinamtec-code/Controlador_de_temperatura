param(
    [string]$HexFile = "Controlador-de-temperatura.hex"
)

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$BuildDir = Join-Path $ProjectRoot "build"
$HexFullPath = Join-Path $BuildDir $HexFile

# Try STM32CubeProgrammer first (official ST tool, more reliable)
$stm32ProgPaths = @(
    "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
    "${env:PROGRAMFILES}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
    "${env:PROGRAMFILES(X86)}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
)

foreach ($path in $stm32ProgPaths) {
    if (Test-Path $path) {
        Write-Host "Using STM32CubeProgrammer: $path"
        Write-Host "Flashing: $HexFullPath"
        & $path -c port=SWD -w $HexFullPath -v
        if ($LASTEXITCODE -eq 0) {
            & $path -c port=SWD -hardrst
            exit 0
        }
        exit 1
    }
}

Write-Error "STM32CubeProgrammer not found. Install from https://www.st.com/en/development-tools/stm32cubeprogrammer.html"