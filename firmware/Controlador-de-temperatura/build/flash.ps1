# Flash STM32 device using STM32CubeProgrammer
param(
    [string]$HexFile = "Controlador-de-temperatura.hex"
)

$stm32ProgPaths = @(
    "C:/Program Files/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
    "${env:PROGRAMFILES}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe",
    "${env:PROGRAMFILES(X86)}/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI.exe"
)

foreach ($path in $stm32ProgPaths) {
    if (Test-Path $path) {
        Write-Host "Using STM32CubeProgrammer: $path"
        & $path -c port=SWD -w $HexFile -v
        if ($LASTEXITCODE -eq 0) {
            & $path -c port=SWD -hardrst
            exit 0
        }
        exit 1
    }
}

Write-Error "STM32CubeProgrammer not found. Install from https://www.st.com/en/development-tools/stm32cubeprogrammer.html"