# Controlador de Temperatura

Proyecto multifase Buck para control de temperatura con firmware reutilizable y documentación del sistema.

## Estructura

```
├── firmware/
│   └── Controlador-de-temperatura/
│       ├── Core/
│       │   ├── Src/
│       │   │   ├── main.c, stm32f3xx_it.c, stm32f3xx_hal_msp.c, system_stm32f3xx.c, syscalls.c
│       │   │   ├── hardware/           # HAL abstractions
│       │   │   │   ├── usart_hw.c, adc_hw.c, hrtim_hw.c, lcd_hw.c, oled_hw.c, i2c_hw.c
│       │   │   ├── tasks/              # Scheduler/task system
│       │   │   │   ├── scheduler.c, task_control.c, task_comm.c, task_system.c, task_ui.c
│       │   │   ├── services/           # Servicios de comunicación
│       │   │   │   ├── scpi_parser.c, error_handler.c, circular_buffer.c
│       │   │   ├── control/            # Control PID
│       │   │   │   ├── pid_controller.c, arm_pid_init_f32.c
│       │   │   └── communication/      # Buffer de comunicación
│       │   │       └── comm_buffers.c
│       │   └── Inc/
│       ├── Drivers/
│       │   ├── STM32F3xx_HAL_Driver/
│       │   └── CMSIS/
│       ├── startup/
│       │   └── startup_stm32f334x8.s
│       ├── CMakeLists.txt
│       ├── toolchain.cmake
│       ├── STM32F334R8Tx_FLASH.ld
│       ├── build/
│       │   ├── build.bat               # Script de compilación (Windows)
│       │   └── flash.ps1               # Script de flashing
│       └── .vscode/
│           ├── tasks.json
│           ├── c_cpp_properties.json
│           └── cmake-kits.json
├── hardware/
│   └── README.md
└── docs/
    ├── main.tex
    ├── IPLeiriaThesis.cls
    └── Matter/
```

## Setup Multiplataforma

### Requisitos (Windows)

1. **GNU Arm Embedded Toolchain 10+** (o Arm GNU Toolchain)
2. **STM32CubeProgrammer** (para flashing con ST-Link)
3. **CMake 3.20+**

### Compilar

```powershell
# Desde el directorio raíz
cd firmware/Controlador-de-temperatura/build
.\build.bat
```

O con CMake + VS Code:
- Ctrl+Shift+P → "CMake: Configure"
- Ctrl+Shift+B → Build

### Flashear

```powershell
# Usando STM32CubeProgrammer CLI
STM32_Programmer_CLI.exe -c port=SWD -w ..\build\Controlador-de-temperatura.hex -r 0x08000000 -v

# O desde VS Code (Ctrl+Shift+P → "Tasks: Run Task" → "Flash Device")
```

## Firmware

STM32F334R8 con:
- HRTIM: Control PWM multifásico (5 canales: A, B, C, D, E) con trigger ADC
- ADC: Lectura temperatura (canal interno en ADC2)
- USART: Comunicación serial con DMA
- Scheduler: Sistema de tareas sin RTOS

Implementa control PID de temperatura con comunicación SCPI a través de UART.

### Comandos UART Soportados (SCPI)

```
*IDN?            - Identificación del dispositivo
*CLS             - Limpiar estado
*RST             - Reset (setpoint=25°C, PID gains=0)
MEAS:TEMP?       - Medir temperatura actual
TEMP:SP?         - Leer setpoint temperatura
TEMP:SP <valor>  - Establecer setpoint (0-200°C)
PID:KP?          - Leer ganancia proporcional
PID:KP <valor>   - Establecer KP
PID:KI?          - Leer ganancia integral
PID:KI <valor>   - Establecer KI
PID:KD?          - Leer ganancia derivativa
PID:KD <valor>   - Establecer KD
SOUR1:OUTP ON/OFF - Control salida 1 (GPIO PC13)
SOUR2:OUTP ON/OFF - Control salida 2 (GPIO PB4)
SOUR1:OUTP?      - Estado salida 1
SOUR2:OUTP?      - Estado salida 2
```

### Estado Actual del Firmware

| Recurso | Uso | Límite | % |
|---------|-----|--------|---|
| FLASH (text) | 25,068 bytes | 64KB | 39% |
| RAM (data+bss) | 6,640 bytes | 12KB | 54% |

Ver `firmware/Controlador-de-temperatura/CHANGES.md` para detalles de implementación y notas técnicas.