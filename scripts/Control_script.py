'''
Script de ejemplo del control de temperatura
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

'''
import pyvisa as vi
import matplotlib.pyplot as plt
import numpy as np
import time



#%% Inicializar el administrador de recursos
rm = vi.ResourceManager()

# Listar recursos disponibles para verificar el nombre del puerto
print("Recursos disponibles:", rm.list_resources())

#%% Abrir el recurso del dispositivo (Puerto Serie 5)
# Se recomienda configurar la terminación al abrir o inmediatamente después
ctemp = rm.open_resource('ASRL5::INSTR')

# Configuración física crítica
ctemp.baud_rate = 115200      # Velocidad correcta detectada
ctemp.data_bits = 8           # Estándar habitual
ctemp.parity = vi.constants.Parity.none
ctemp.stop_bits = vi.constants.StopBits.one
ctemp.flow_control = 0        # Generalmente 0 (ninguno) para instrumentos

# Configuración de protocolo (Terminadores)
ctemp.write_termination = '\n'
ctemp.read_termination = '\n'
ctemp.timeout = 2000          # 2 segundos es suficiente a esta velocidad

#%% Prueba de comunicación
try:
    idn = ctemp.query('*IDN?')
    print(f"Dispositivo conectado: {idn}")
    
    # Ahora puedes ejecutar el resto de tu script de temperatura
    temp = ctemp.query('MEAS:TEMP?')
    print(f"Temperatura: {temp}")
    
except vi.VisaIOError as e:
    print(f"Error de comunicación: {e}")

#%% Apagar salididas,
def apagar_salidas():
    ctemp.write('SOUR1:OUTP OFF')
    time.sleep(.1)
    ctemp.write('SOUR2:OUTP OFF')
    time.sleep(.1)
    ctemp.flush(vi.constants.VI_READ_BUF)
#%% encender salididas,
def encender_salidas():
    ctemp.write('SOUR1:OUTP ON')
    time.sleep(.1)
    ctemp.write('SOUR2:OUTP ON')
    time.sleep(.1)
    ctemp.flush(vi.constants.VI_READ_BUF)
#%% Medición de la temperatura
temp = []
duty = []
tiempo = []

duracion_de_medicion = 10
periodo_de_muestreo = 0.2 
i = 0

try:
    encender_salidas()
    time.sleep(0.1) 

    # --- Configuración del Gráfico con Subplots ---
    fig, (ax1, ax2) = plt.subplots(2, 1, sharex=True)
    plt.ion()  # Modo interactivo para actualizar en tiempo real
    
    # Configuración eje 1 (Temperatura)
    ax1.set_ylabel('Temperatura (°C)', color='blue')
    ax1.tick_params(axis='y', labelcolor='blue')
    ax1.grid(True, linestyle='--', alpha=0.6)
    line_temp, = ax1.plot([], [], 'b-', label='Temp') # Línea azul
    ax1.legend(loc='upper left')

    # Configuración eje 2 (Duty Cycle)
    ax2.set_ylabel('Duty Cycle (%)', color='red')
    ax2.tick_params(axis='y', labelcolor='red')
    ax2.grid(True, linestyle='--', alpha=0.6)
    ax2.set_xlabel('Tiempo (s)')
    line_duty, = ax2.plot([], [], 'r-', label='Duty') # Línea roja
    ax2.legend(loc='upper left')

    t_init = time.time()
    print("Medición iniciada...")

    while True:
        lapso = time.time() - t_init
        if lapso >= duracion_de_medicion:
            break
            
        if lapso > i * periodo_de_muestreo:
            i += 1
            
            # 1. Leer Temperatura
            try:
                respuesta = ctemp.query('MEAS:TEMP?')
                valor_temp = float(respuesta.strip())
                temp.append(valor_temp)
                tiempo.append(lapso)
                print(f"T={lapso:.2f}s -> {valor_temp}°C")
            except ValueError:
                print(f"Error Temp: '{respuesta}'")
                ctemp.flush(vi.constants.VI_READ_BUF)
                continue # Saltar este ciclo si falla la lectura principal

            # 2. Leer Duty Cycle
            try:
                respuesta = ctemp.query('PID:DUTY?')
                valor_duty = float(respuesta.strip())
                duty.append(valor_duty)
                # No append a 'tiempo' de nuevo, ya se añadió arriba
            except ValueError:
                print(f"Error Duty: '{respuesta}'")
                duty.append(np.nan) # Mantener alineación con NaN si falla
                ctemp.flush(vi.constants.VI_READ_BUF)

            # --- Actualizar Gráficos ---
            line_temp.set_data(tiempo, temp)
            line_duty.set_data(tiempo, duty)
            
            # Ajustar límites para ver los nuevos datos
            ax1.relim()
            ax1.autoscale_view(scaley=True)
            ax2.relim()
            ax2.autoscale_view(scaley=True)
            
            plt.pause(0.01)

    plt.ioff() # Desactivar modo interactivo al finalizar
    plt.show() # Mostrar ventana final interactiva
    apagar_salidas()

except vi.VisaIOError as e:
    print(f"Error Visa: {e}")
except Exception as e:
    print(f"Error general: {e}")
finally:
    # Asegurar que se apaguen las salidas incluso si hay error
    try:
        apagar_salidas()
    except:
        pass