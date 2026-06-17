'''
Copmandos
    *IDN?            - Identificación del dispositivo
    *CLS             - Limpiar estado
    *RST             - Reset (setpoint=25°C, PID gains=0)
    MEAS:TEMP?       - Medir temperatura actual
    TEMP:SP?         - Leer setpoint temperatura
    TEMP:SP <valor>  - Establecer setpoint (0-200°C)
    PID:KP?          - Leer ganancia proporcional
    PID:KP <valor>   - Establecer KP en 1/C°
    PID:KI?          - Leer ganancia integral 1/(C° min)
    PID:KI <valor>   - Establecer KI
    PID:KD?          - Leer ganancia derivativa
    PID:KD <valor>   - Establecer KD
    SOUR1:OUTP ON/OFF - Control salida 1 (GPIO PC13)
    SOUR2:OUTP ON/OFF - Control salida 2 (GPIO PB4)
    SOUR1:OUTP?      - Estado salida 1
    SOUR2:OUTP?      - Estado salida 2

'''
'''
Script para el control de temperatura, visualización en tiempo real 
y exportación de datos con Pandas.
'''
import pyvisa as vi
import matplotlib.pyplot as plt
import numpy as np
import time
import pandas as pd
import time

# ==========================================
#%% 1. FUNCIONES AUXILIARES
# ==========================================
def apagar_salidas(instrumento):
    instrumento.write('SOUR1:OUTP OFF')
    time.sleep(.1)
    instrumento.write('SOUR2:OUTP OFF')
    time.sleep(.1)
    instrumento.flush(vi.constants.VI_READ_BUF)

def encender_salidas(instrumento):
    instrumento.write('SOUR1:OUTP ON')
    time.sleep(.1)
    instrumento.write('SOUR2:OUTP ON')
    time.sleep(.1)
    instrumento.flush(vi.constants.VI_READ_BUF)
    
def query_con_reintento(instrumento, comando, reintentos=3, pausa=1):
    """
    Envía un comando query. Si hay un timeout de Visa, limpia buffers y reintenta.
    """
    for intento in range(1, reintentos + 1):
        try:
            return instrumento.query(comando).strip()
        except vi.VisaIOError as e:
            # Si es el último intento, lanzamos el error para que el programa lo ataje
            if intento == reintentos:
                raise e
            
            # Si falló, limpiamos buffers para no leer basura en el próximo intento
            instrumento.flush(vi.constants.VI_READ_BUF)
            instrumento.flush(vi.constants.VI_WRITE_BUF)
            
            # Pequeña pausa antes del próximo intento
            time.sleep(pausa)
            print(f"    [Advertencia] Reintento {intento}/{reintentos} para '{comando}'...")

# ==========================================
#%% 2. CONFIGURACIÓN DEL INSTRUMENTO
# ==========================================
rm = vi.ResourceManager()
print("Recursos disponibles:", rm.list_resources())

ctemp = rm.open_resource('ASRL6::INSTR')

# Configuración física
ctemp.baud_rate = 115200
ctemp.data_bits = 8
ctemp.parity = vi.constants.Parity.none
ctemp.stop_bits = vi.constants.StopBits.one
ctemp.flow_control = 0

# Configuración de protocolo
ctemp.write_termination = '\n'
ctemp.read_termination = '\n'
ctemp.timeout = 5000

# ==========================================
#%% 3. PRUEBA DE COMUNICACIÓN Y SETEO INICIAL
# ==========================================
try:
    idn = ctemp.query('*IDN?').strip()
    print(f"Dispositivo conectado: {idn}")
    
    # Verificación de parámetros actuales
    tinit = time.time()
    temp_val,kp,ki,kd,sp = [float(x) for x in (ctemp.query('MEAS:TEMP?;PID:KP?;PID:KI?;PID:KD?;TEMP:SP?').split(';'))]
    dt = time.time()-tinit
    print (f"Duracion de llamada {dt}segundos")
    print(f"Temperatura anterior: {temp_val:.2f} °C")
    print(f"El setpoint anterior: {sp:.2f} °C")
    print(f"Kp anterior: {kp:.4f}")
    print(f"Ki anterior: {ki:.4f}")
    print(f"Kd anterior: {kd:.4f}")

    # Configurar constantes PID iniciales
    ctemp.write('PID:KP 5.0;PID:KI 25.0;PID:KD 0.0')
    time.sleep(.1)
    temp_val,kp,ki,kd,sp = [float(x) for x in (ctemp.query('MEAS:TEMP?;PID:KP?;PID:KI?;PID:KD?;TEMP:SP?').split(';'))]
    print(f"La temperatura actual: {temp_val:.2f} °C")
    print(f"El setpoint actual: {sp:.2f} °C")
    print(f"Kp actual: {kp:.4f}")
    print(f"Ki actual: {ki:.4f}")
    print(f"Kd actual: {kd:.4f}")

except Exception as e:
    print(f"Error durante la inicialización: {e}")

# ==========================================
#%% 4. MEDICIÓN Y GRAFICACIÓN EN TIEMPO REAL
# ==========================================
temp, duty, tiempo, error_temp = [], [], [], []

# Parámetros de la prueba
setpoint = 5.0 
duracion_de_medicion = 20  # Segundos
periodo_de_muestreo = 0.15   # Segundos

ctemp.write(f'TEMP:SP {setpoint}')
time.sleep(.5)

try:
    encender_salidas(ctemp)

    # --- Configuración del Gráfico ---
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, sharex=True, figsize=(8, 8)) 
    fig.tight_layout(pad=3.0)
    plt.ion()
    
    # Eje 1 (Temperatura)
    ax1.set_ylabel('Temperatura (°C)', color='blue')
    ax1.grid(True, linestyle='--', alpha=0.6)
    line_temp, = ax1.plot([], [], 'b-', label='Temp')
    ax1.axhline(y=setpoint, color='green', linestyle='--', label=f'SP ({setpoint}°C)')
    ax1.legend(loc='upper left')

    # Eje 2 (Duty Cycle)
    ax2.set_ylabel('Duty Cycle (%)', color='red')
    ax2.grid(True, linestyle='--', alpha=0.6)
    line_duty, = ax2.plot([], [], 'r-', label='Duty')
    ax2.legend(loc='upper left')

    # Eje 3 (Error)
    ax3.set_ylabel('Error (°C)', color='purple')
    ax3.set_xlabel('Tiempo (s)')
    ax3.grid(True, linestyle='--', alpha=0.6)
    ax3.axhline(y=0, color='gray', linestyle=':', alpha=0.7) 
    line_error, = ax3.plot([], [], '-', color='purple', label='Error (SP - Temp)')
    ax3.legend(loc='upper left')

    t_init = time.time()
    i = 0
    print("\nIniciando medición...")

    while True:
        lapso = time.time() - t_init
        if lapso >= duracion_de_medicion:
            print("Medición completada por tiempo.")
            break
            
        if lapso > i * periodo_de_muestreo:
            i += 1
            
            # 1. Leer Temperatura y Calcular Error
            try:
                # Usamos la nueva función con reintentos
                respuesta = query_con_reintento(ctemp, 'MEAS:TEMP?', reintentos=3)
                valor_temp = float(respuesta)
                valor_error = setpoint - valor_temp
                
                temp.append(valor_temp)
                tiempo.append(lapso)
                error_temp.append(valor_error)
                
                print(f"T={lapso:.1f}s | Temp={valor_temp}°C | Error={valor_error:.2f}°C")
            except vi.VisaIOError:
                print("Error: Pérdida temporal de comunicación al leer Temp. Saltando ciclo.")
                continue # Saltamos el ciclo sin crashear
            except ValueError:
                print(f"Error de formato al leer Temp: '{respuesta}'")
                continue

            # 2. Leer Duty Cycle
            try:
                respuesta_duty = query_con_reintento(ctemp, 'PID:DUTY?', reintentos=3)
                duty.append(float(respuesta_duty))
            except (vi.VisaIOError, ValueError):
                print("Error al leer Duty. Asignando NaN.")
                duty.append(np.nan)

            # --- Actualizar Gráficos ---
            line_temp.set_data(tiempo, temp)
            line_duty.set_data(tiempo, duty)
            line_error.set_data(tiempo, error_temp)
            
            for ax in (ax1, ax2, ax3):
                ax.relim()
                ax.autoscale_view(scaley=True)
            
            plt.pause(0.01)

except KeyboardInterrupt:
    print("\nMedición interrumpida por el usuario.")
except vi.VisaIOError as e:
    print(f"\nError de comunicación Visa: {e}")
except Exception as e:
    print(f"\nError general: {e}")
finally:
    print("\nEjecutando rutina de apagado y guardado seguro...")
    
    # ----------------------------------------
    # BLOQUE DE GUARDADO CON PANDAS
    # ----------------------------------------
    try:
        if len(tiempo) > 0:
            # Recortar al largo mínimo por si el script se cortó a la mitad de un ciclo
            min_len = min(len(tiempo), len(temp), len(duty), len(error_temp))
            
            df = pd.DataFrame({
                'Tiempo_s': tiempo[:min_len],
                'Temperatura_C': temp[:min_len],
                'Duty_Cycle_pct': duty[:min_len],
                'Error_C': error_temp[:min_len]
            })
            
            # Generamos un nombre de archivo único con fecha y hora
            timestamp = time.strftime("%Y%m%d_%H%M%S")
            nombre_archivo = f"medicion_PID_{timestamp}.csv"
            
            df.to_csv(nombre_archivo, index=False)
            print(f"-> Datos exportados exitosamente a: {nombre_archivo}")
        else:
            print("-> No hubo datos suficientes para generar un archivo CSV.")
    except Exception as e:
        print(f"-> Error al intentar guardar los datos: {e}")
        
    # ----------------------------------------
    # APAGADO DEL EQUIPO
    # ----------------------------------------
    try:
        ctemp.write('TEMP:SP 30.0')
        time.sleep(0.1)
        apagar_salidas(ctemp)
        print("-> Salidas apagadas.")
    except Exception as e:
        print(f"-> Error al intentar cerrar el equipo: {e}")