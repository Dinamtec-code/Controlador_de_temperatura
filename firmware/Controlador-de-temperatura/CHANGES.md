# Controlador de Temperatura - Firmware STM32

## Arquitectura del Firmware

```
Core/Src/
├── main.c                  # Punto de entrada, inicialización
├── hardware/               # Abstracciones de hardware
│   ├── usart_hw.c         # UART con DMA buffer
│   ├── adc_hw.c           # Lectura temperatura ADC2
│   ├── hrtim_hw.c         # Control PWM HRTIM
│   ├── lcd_hw.c, oled_hw.c
│   └── i2c_hw.c
├── tasks/                  # Sistema de tareas (scheduler)
│   ├── scheduler.c        # Scheduler simple sin RTOS
│   ├── task_comm.c        # Procesamiento UART/SCPI
│   ├── task_control.c     # Control PID temperatura
│   ├── task_system.c
│   └── task_ui.c
├── services/               # Servicios
│   ├── scpi_parser.c      # Parser SCPI para UART
│   ├── error_handler.c
│   └── circular_buffer.c
├── control/                # Control PID
│   ├── pid_controller.c
│   └── arm_pid_init_f32.c
└── communication/
    └── comm_buffers.c      # Buffer UART circular
```

## Características Implementadas

### Control PID de Temperatura
- Setpoint configurable: 0-200°C
- Ganancia KP, KI, KD ajustable vía SCPI
- Límites de salida: 0.5% - 49.5%
- Salida PWM controlada en canales TA1/TB1

### HRTIM Multifásico
- 5 canales PWM (TA1, TB1, TC1, TD1, TE1)
- Dead time: 200 ticks (prescaler ×8)
- ADC trigger desde master timer

### UART SCPI
- Comunicación a 115200 baudios
- Buffer circular con DMA
- Respuestas formateadas

## Build System

### Herramientas Necesarias (Windows)
1. **GNU Arm Embedded Toolchain 10+** (o Arm GNU Toolchain)
2. **STM32CubeProgrammer**: `C:\Program Files\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe`
3. **CMake 3.20+**

### Compilar

```powershell
# Desde el directorio firmware/Controlador-de-temperatura/build
.\build.bat

# O con CMake + VS Code
# Ctrl+Shift+P → "CMake: Configure"
# Ctrl+Shift+B → Build
```

### Flashear

```powershell
STM32_Programmer_CLI.exe -c port=SWD -w ..\build\Controlador-de-temperatura.hex -r 0x08000000 -v
```

## Estado Actual del Firmware

| Recurso | Uso | Límite | % |
|---------|-----|--------|---|
| FLASH (text) | 25,068 bytes | 64KB | 39% |
| RAM (data+bss) | 6,640 bytes | 12KB | 54% |

## Comandos UART Soportados (SCPI)

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

## Notas de Configuración HRTIM
- Clock: 144 MHz (PLL * 9 / 2)
- Periodo Master: 64000 → frecuencia ≈ 2.25 kHz
- Dead time: 200 ticks (prescaler ×8)
- Pin mapping: CHA1/PA8, CHB1/PA10, CHC1/PB12, CHD1/PB14, CHE1/PC8