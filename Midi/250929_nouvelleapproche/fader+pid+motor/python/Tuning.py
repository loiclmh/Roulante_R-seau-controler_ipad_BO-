"""
Tuning.py — lance automatiquement les essais PID (Pico) et sauvegarde la figure.
- Envoie Kp/Ki/Kd/Ts/fc au Pico via SLIP pour **un fader choisi globalement** (fader_idx)
- Envoie 's<i>' pour démarrer l'essai du fader i (speed_div)
- Log les données dans ./data/*.tsv
- Trace 1 figure avec 4 sous-graphiques et SAUVEGARDE en .svg et .png
"""

import struct
from numpy import genfromtxt
import matplotlib.pyplot as plt
from sys import path
from serial import Serial
from time import sleep
from os.path import isfile
from os import chdir, makedirs
from pathlib import Path
from datetime import datetime
from serial.tools import list_ports
import glob, sys
from SLIP import read_slip, write_slip

# ------------------ Réglages ------------------
BAUDRATE = 1_000_000
TIMEOUT  = 0.5

def autodetect_port() -> str | None:
    """Détection auto du port (macOS/Linux/Windows). Préférence RP2040 (VID 0x2E8A)."""
    # Essais rapides via glob
    cands = sorted(
        glob.glob('/dev/tty.usbmodem*') +
        glob.glob('/dev/tty.usbserial*') +
        glob.glob('/dev/ttyACM*') +
        glob.glob('/dev/ttyUSB*')
    )
    # PySerial: si on peut identifier le Pico par VID
    pico = None
    for p in list_ports.comports():
        if getattr(p, 'vid', None) == 0x2E8A:  # Raspberry Pi RP2040
            pico = p.device
            break
        name = (p.device or '')
        if any(k in name for k in ('usbmodem','usbserial','ttyACM','ttyUSB')):
            pico = pico or name
    if pico:
        return pico
    if cands:
        return cands[0]
    if sys.platform.startswith('win'):
        return 'COM3'  # dernier recours
    return None

SERIAL_PORT = autodetect_port()
if SERIAL_PORT is None:
    raise RuntimeError("Aucun port série détecté (usbmodem/usbserial/ACM/USB). Branche le Pico, ferme le Moniteur série Arduino, puis relance.")
print(f"[INFO] Port série utilisé : {SERIAL_PORT}")

# =================== Jeux de paramètres  à modifier Idx + (Kp, Ki, Kd, ts, fc) ===================
fader_idx = 1  # index du fader/moteur à tester (0, 1, 2, ...)

tunings = [
    (6,  2,   0.035, 0.001, 60),
    (6,  8,   0.035, 0.001, 60),
    (6,  64,  0.035, 0.001, 60),
    (6,  256, 0.035, 0.001, 60),
]

# =================== Jeux de paramètres  à modifier Idx + (Kp, Ki, Kd, ts, fc) ===================

# Diviseur de vitesse du profil côté Pico (utilisé dans le 's')
speed_div = 8.0
sma = False                # conservé pour compat
img_basename = 'tuning_result'

# --- Sélection du fader/moteur ---
# On choisit le fader globalement avec fader_idx, et chaque tuning est un 5-tuple.

# --- Sélection du fader/moteur via un index global ---
# Chaque tuning DOIT être un 5-tuple: (Kp, Ki, Kd, Ts, Fc)

def parse_tuning(t: tuple):
    if len(t) != 5:
        raise ValueError("Chaque tuning doit être un 5-tuple: (Kp,Ki,Kd,Ts,Fc)")
    idx = int(fader_idx)
    Kp, Ki, Kd, Ts, Fc = map(float, t)
    return idx, Kp, Ki, Kd, Ts, Fc

def get_tuning_name(tuning: tuple):
    idx, Kp, Ki, Kd, Ts, Fc = parse_tuning(tuning)
    name  = f"i{idx}-p{Kp}i{Ki}d{Kd}t{Ts}c{Fc}"
    name += 's' + str(speed_div)
    if sma:
        name += '-sma'
    return name

def get_readable_name(tuning: tuple):
    idx, Kp, Ki, Kd, Ts, Fc = parse_tuning(tuning)
    name = f"[i{idx}] $K_p = {Kp:.2f}$,     $K_i = {Ki:.2f}$,     $K_d = {Kd:.4f}$,     $T_s = {Ts:.4f}$ s,     $f_c = {Fc:.2f}$"
    if sma:
        name += ' (SMA)'
    return name

def set_tuning(ser: Serial, tuning: tuple):
    """Envoie Kp, Ki, Kd, Ts, Fc pour l'index choisi en SLIP.
    Format Pico: b'<key><idx>' + float32 LE, keys = ('p','i','d','t','c').
    """
    idx, Kp, Ki, Kd, Ts, Fc = parse_tuning(tuning)
    keys = ('p', 'i', 'd', 't', 'c')
    values = (Kp, Ki, Kd, Ts, Fc)
    for key, value in zip(keys, values):
        payload = (key + str(idx)).encode() + struct.pack('<f', float(value))
        write_slip(ser, payload)
        sleep(0.01)

def start(ser: Serial, tuning: tuple):
    idx, *_ = parse_tuning(tuning)
    msg = ('s' + str(idx)).encode() + struct.pack('<f', speed_div)
    write_slip(ser, msg)

# ------------------ Script principal ------------------
root = Path(__file__).parent
chdir(root)
DATA_DIR    = root / 'data'
RESULTS_DIR = root / 'results'
DATA_DIR.mkdir(exist_ok=True)
RESULTS_DIR.mkdir(exist_ok=True)

fig, axs = plt.subplots(len(tunings), 1, sharex='all', sharey='all',
                        figsize=(12, 8), squeeze=False)

with Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT) as ser:
    for i, tuning in enumerate(tunings):
        print(get_readable_name(tuning))
        filename = DATA_DIR / ('data' + get_tuning_name(tuning) + '.tsv')

        with open(str(filename), 'w') as f:
            ser.reset_input_buffer()

            # Applique les PID
            set_tuning(ser, tuning)
            read_slip(ser)              # vide une éventuelle réponse
            ser.reset_input_buffer()

            # Démarre automatiquement l'expérience (pas d'attente longue)
            start(ser, tuning)

            # Lecture des frames jusqu'à None (fin d'essai)
            while True:
                data = read_slip(ser)
                if data is None:
                    break
                f.write('\t'.join(map(str, data)) + '\n')

        # Charge et trace le fichier
        data = genfromtxt(filename, delimiter='\t')
        try:
            n = getattr(data, 'shape', (0,))[0]
        except Exception:
            n = 0
        if n == 0:
            print(f"[WARN] Fichier vide: {filename} — aucun tracé.")
            continue
        axs[i][0].plot(data, linewidth=1)
        axs[i][0].axhline(0, linewidth=0.5, color='k')
        axs[i][0].set_title(get_readable_name(tuning))

# Sauvegarde des figures dans ./results
RESULTS_DIR.mkdir(exist_ok=True)
timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
svg_path = RESULTS_DIR / f"{img_basename}-{timestamp}.svg"
png_path = RESULTS_DIR / f"{img_basename}-{timestamp}.png"
plt.tight_layout()
plt.savefig(str(svg_path))
plt.savefig(str(png_path), dpi=300)
print("Figures sauvegardées :", svg_path, png_path)

plt.show()