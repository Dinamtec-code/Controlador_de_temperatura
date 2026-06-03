# Controlador de Temperatura

Proyecto multifase Buck para control de temperatura con firmware reutilizable y documentación del sistema.

## Estructura

```
├── firmware/
│   └── Controlador-de-temperatura/
│       ├── Core/           # Código fuente HAL
│       ├── Drivers/        # CMSIS y HAL drivers
│       ├── startup/        # Código de arranque
│       ├── CMakeLists.txt  # Build system
│       ├── build.bat       # Build script (Windows)
│       ├── flash.ps1       # Flash script (auto-detect OpenOCD)
│       └── setup.ps1       # Download OpenOCD portable if needed
├── hardware/
│   ├── pcb/              # (futuro) Esquemáticos, gerber, imágenes PCB
│   ├── mecanical/        # (futuro) Planos STL, archivos Inventor
│   └── README.md         # Índice de archivos de hardware
└── docs/
    └── main.tex          # Documentación del sistema (LaTeX)
```

## Setup Multiplataforma

Requisitos:
- GNU Arm Embedded Toolchain (10.3 o posterior)
- CMake
- OpenOCD (auto-detección o descarga automática)

```powershell
# Setup automático (descarga OpenOCD si no existe)
cd firmware/Controlador-de-temperatura
.\setup.ps1

# Compilar
.\build.bat

# Flashear (detecta OpenOCD de AC6, STM32CubeProgrammer o portable)
.\flash.ps1
```

## Firmware

STM32F334R8 con:
- HRTIM: Control PWM multifase (5 canales)
- ADC: Lectura triggered por HRTIM_TRG1
- UART: Comunicación con IDLE DMA

Comandos UART:
```
S,A    - Start PWM
S,O    - Stop PWM  
X,1234 - Set PWM eje X
Y,567  - Set PWM eje Y
Z,890  - Set PWM eje Z
P,180  - Set fase
F,150k - Set frecuencia (ej: 150000)
```

Ver `firmware/Controlador-de-temperatura/CHANGES.md` para detalles de implementación.