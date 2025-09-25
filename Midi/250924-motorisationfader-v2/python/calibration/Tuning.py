"""
Tuning.py — lance automatiquement les essais PID (Pico) et sauvegarde la figure.
- Envoie Kp/Ki/Kd/fc au Pico via SLIP
- Envoie 's' pour démarrer chaque essai
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

# Jeux de paramètres (Kp, Ki, Kd, fc)
tunings = [
    (6,  2,   0.035, 60),
    (6,  8,   0.035, 60),
    (6,  64,  0.035, 60),
    (6,  256, 0.035, 60),
]

# Diviseur de vitesse du profil côté Pico (utilisé dans le 's')
speed_div = 8.0
fader_idx = 0
sma = False                # conservé pour compat
img_basename = 'tuning_result'

# ------------------ Helpers ------------------
def get_tuning_name(tuning: tuple((float, float, float, float))):
    name = ''
    for i in range(0, 4):
        setting = ('p', 'i', 'd', 'c')[i]
        set_k = setting + str(tuning[i])
        name += set_k
    name += 's' + str(speed_div)
    if sma: name += '-sma'
    if len(tuning) > 4: name += tuning[4]
    return name

def get_readable_name(tuning: tuple((float, float, float, float))):
    name = '$K_p = {:.2f}$,     $K_i = {:.2f}$,     $K_d = {:.4f}$,     $f_c = {:.2f}$'.format(*tuning)
    if sma: name += ' (SMA)'
    return name

def set_tuning(ser: Serial, tuning: tuple((float, float, float, float))):
    for i in range(0, 4):
        setting = ('p', 'i', 'd', 'c')[i]
        set_k = (setting + str(fader_idx)).encode()
        set_k += struct.pack('<f', tuning[i])
        write_slip(ser, set_k)
        sleep(0.01)

def start(ser: Serial):
    msg = b's0' + struct.pack('<f', speed_div)
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
            start(ser)

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