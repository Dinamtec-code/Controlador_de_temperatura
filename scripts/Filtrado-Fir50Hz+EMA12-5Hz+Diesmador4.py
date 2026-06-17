import numpy as np
import scipy.signal as signal
import matplotlib.pyplot as plt

# ==========================================
# PARÁMETROS DEL SISTEMA
# ==========================================
fs_original = 72000.0  # Frecuencia de muestreo del hardware (Hz)
fc_deseada = 10.0      # Frecuencia de corte deseada (Hz)
orden_filtro = 4       # Etapas del filtro EMA en cascada

# Parámetros del Filtro Notch final (Media móvil en la ISR del DMA)
fs_notch = 1000.0      # Se ejecuta cada 1ms (1 kHz)
N_notch = 20           # 20 muestras para clavar el cero en 50 Hz

# Límite de cuantización/resolución para un float32 (23 bits de mantisa)
eps_float32 = 1.0 / (2**23)

# Vector de frecuencias f a evaluar para el dominio de la frecuencia
f = np.logspace(-1, np.log10(fs_original / 2.0), 10000)

# ==========================================
# FUNCIONES DE ANÁLISIS
# ==========================================
def calcular_respuesta_sistema(M):
    """Calcula la respuesta en frecuencia (magnitud) del sistema."""
    # 1. Primer Promediador (Hardware)
    with np.errstate(divide='ignore', invalid='ignore'):
        mag_ma = np.abs(np.sin(np.pi * f * M / fs_original) / (M * np.sin(np.pi * f / fs_original)))
    mag_ma[np.isnan(mag_ma)] = 1.0

    # 2. Filtro EMA (IIR @ Fs/M)
    fs_diezmada = fs_original / M
    correccion_cascada = np.sqrt(2**(1.0 / orden_filtro) - 1.0)
    alpha = 1.0 - np.exp(-2.0 * np.pi * (fc_deseada / fs_diezmada) / correccion_cascada)
    
    z_inv = np.exp(-1j * 2.0 * np.pi * f / fs_diezmada)
    H_ema_una_etapa = alpha / (1.0 - (1.0 - alpha) * z_inv)
    H_ema_cascada = H_ema_una_etapa ** orden_filtro
    mag_ema = np.abs(H_ema_cascada)

    # 3. Filtro Notch Final (Media Móvil @ 1 kHz)
    with np.errstate(divide='ignore', invalid='ignore'):
        mag_notch = np.abs(np.sin(np.pi * f * N_notch / fs_notch) / (N_notch * np.sin(np.pi * f / fs_notch)))
    mag_notch[np.isnan(mag_notch)] = 1.0

    mag_total = np.maximum(mag_ma * mag_ema * mag_notch, 1e-10) 
    db_total = 20 * np.log10(mag_total)
    
    return db_total, mag_total

def calcular_respuesta_escalon(M, t_max=0.4):
    """Simula la respuesta temporal del sistema completo ante un escalón."""
    fs_diezmada = fs_original / M
    num_muestras_ema = int(t_max * fs_diezmada)
    
    u = np.ones(num_muestras_ema)
    
    correccion_cascada = np.sqrt(2**(1.0 / orden_filtro) - 1.0)
    alpha = 1.0 - np.exp(-2.0 * np.pi * (fc_deseada / fs_diezmada) / correccion_cascada)
    b_ema = [alpha]
    a_ema = [1.0, -(1.0 - alpha)]
    
    y_ema = u.copy()
    for _ in range(orden_filtro):
        y_ema = signal.lfilter(b_ema, a_ema, y_ema)
        
    factor_decimacion = int(fs_diezmada / fs_notch)
    y_decimada = y_ema[::factor_decimacion]
    
    b_notch = np.ones(N_notch) / N_notch
    a_notch = [1.0]
    y_final = signal.lfilter(b_notch, a_notch, y_decimada)
    
    t_ms = (np.arange(len(y_final)) / fs_notch) * 1000.0
    return t_ms, y_final

# ==========================================
# CÁLCULOS
# ==========================================
db_M2, mg_M2 = calcular_respuesta_sistema(M=2)
db_M4, mg_M4 = calcular_respuesta_sistema(M=4)

t_ms_M2, y_M2 = calcular_respuesta_escalon(M=2)
t_ms_M4, y_M4 = calcular_respuesta_escalon(M=4)

# ==========================================
# SUBPLOTS
# ==========================================
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 10))

# --- SUBPLOT 1: Respuesta en Frecuencia (Log-Log) ---
ax1.loglog(f, mg_M2, label='M=2 + EMA @ 36 kHz + Notch 50Hz', linewidth=2)
ax1.loglog(f, mg_M4, label='M=4 + EMA @ 18 kHz + Notch 50Hz (Tu sistema)', linewidth=2, linestyle='--')
ax1.axvline(fc_deseada, color='red', linestyle=':', label=f'Corte EMA ({fc_deseada} Hz)')
ax1.axvline(50.0, color='purple', linestyle='-.', alpha=0.7, label='50 Hz (Ruido de Red)')
ax1.axhline(0.707, color='gray', linestyle=':', label='Atenuación -3 dB (0.707)')

# AGREGADO: Línea de límite físico para float32
ax1.axhline(eps_float32, color='crimson', linestyle='--', alpha=0.8, linewidth=1.5,
            label=f'Límite de Mantisa Float32 ($2^{{-23}} \\approx {eps_float32:.2e}$)')

ax1.set_title('Análisis Frecuencial: Amplitud Lineal del Sistema Completo', fontsize=13)
ax1.set_xlabel('Frecuencia [Hz]', fontsize=11)
ax1.set_ylabel('Ganancia de Amplitud [V/V]', fontsize=11)
ax1.set_ylim(1e-9, 1.5)
ax1.set_xlim(0.1, fs_original / 2.0)
ax1.grid(True, which="both", ls="-", alpha=0.4)
ax1.legend(loc='lower left')

ax1.text(55, 1e-4, 'Cero profundo (Notch)\nen 50 Hz y armónicos', 
         horizontalalignment='left', verticalalignment='center', bbox=dict(facecolor='white', alpha=0.8, edgecolor='none'))

# --- SUBPLOT 2: Respuesta Temporal al Escalón (Lineal) ---
ax2.plot(t_ms_M2, y_M2, label='M=2 (Filtro rápido)', linewidth=2)
ax2.plot(t_ms_M4, y_M4, label='M=4 (Tu sistema)', linewidth=2, linestyle='--')
ax2.axhline(1.0, color='black', linestyle='-', alpha=0.3, label='Valor Estacionario (1.0)')
ax2.axhline(0.90, color='orange', linestyle=':', alpha=0.7, label='90% de Asentamiento')

idx_90 = np.where(y_M4 >= 0.90)[0][0]
t_90 = t_ms_M4[idx_90]
ax2.axvline(t_90, color='orange', linestyle=':', alpha=0.7)

ax2.set_title('Análisis Temporal: Respuesta al Escalón Unitario (Filtro Total)', fontsize=13)
ax2.set_xlabel('Tiempo [ms]', fontsize=11)
ax2.set_ylabel('Amplitud de Salida []', fontsize=11)
ax2.set_xlim(0, 350)
ax2.set_ylim(-0.05, 1.15)
ax2.grid(True, which="both", ls="-", alpha=0.4)
ax2.legend(loc='lower right')
ax2.text(t_90 + 5, 0.4, f'Asentamiento al 90%:\n~{t_90:.1f} ms', 
         horizontalalignment='left', color='darkorange', weight='bold')

plt.tight_layout()
plt.show()