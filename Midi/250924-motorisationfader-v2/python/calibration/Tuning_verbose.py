"""
Tuning_verbose.py — version détaillée (verbose) pour suivre les essais PID
- Auto-détection du port /dev/tty.usbmodem* sur macOS (ou configure SERIAL_PORT)
- Envoie Kp/Ki/Kd/fc au Pico via SLIP, puis 's' pour démarrer
- Log des données dans ./data/*.tsv
- Trace les courbes et sauvegarde automatiquement en .svg et .png
"""
import struct, glob, sys
from numpy import genfromtxt
import matplotlib.pyplot as plt
from sys import path
from serial import Serial, SerialException
from time import sleep
from os.path import isfile
from os import chdir, makedirs
from datetime import datetime
from SLIP import read_slip, write_slip

# ------------------ Réglages ------------------
SERIAL_PORT = 'auto'  # 'auto' pour macOS, ou mets '/dev/tty.usbmodem1101' etc.
BAUDRATE    = 1_000_000
TIMEOUT     = 0.5

# Paramètres PID à tester (Kp, Ki, Kd, fc)
tunings = [
    (6,  2,   0.035, 60),
    (6,  8,   0.035, 60),
    (6,  64,  0.035, 60),
    (6,  256, 0.035, 60),
]

speed_div = 8.0
fader_idx = 0
sma = False
img_basename = 'tuning_verbose'

def auto_port():
    cands = sorted(glob.glob('/dev/tty.usbmodem*') + glob.glob('/dev/tty.usbserial*'))
    return cands[0] if cands else None

def get_tuning_name(tuning):
    name = ''
    for i in range(0, 4):
        setting = ('p', 'i', 'd', 'c')[i]
        name += setting + str(tuning[i])
    name += 's' + str(speed_div)
    if sma: name += '-sma'
    return name

def get_readable_name(tuning):
    return '$K_p = {:.2f}$, $K_i = {:.2f}$, $K_d = {:.4f}$, $f_c = {:.2f}$'.format(*tuning)

def set_tuning(ser, tuning):
    print(f"  -> Envoi des coefficients PID {tuning}")
    for i in range(0, 4):
        setting = ('p', 'i', 'd', 'c')[i]
        cmd = (setting + str(fader_idx)).encode() + struct.pack('<f', tuning[i])
        write_slip(ser, cmd)
        sleep(0.01)

def start(ser):
    print("  -> Démarrage de l’expérience")
    msg = b's0' + struct.pack('<f', speed_div)
    write_slip(ser, msg)

# ------------------ Script principal ------------------
chdir(path[0])
makedirs('data', exist_ok=True)

if SERIAL_PORT == 'auto':
    detected = auto_port()
    if not detected:
        print("[ERREUR] Aucun port série détecté (/dev/tty.usbmodem*). Branche le Pico et réessaie.")
        sys.exit(1)
    SERIAL_PORT = detected

print(f"[INFO] Ouverture du port série {SERIAL_PORT} @ {BAUDRATE}")

fig, axs = plt.subplots(len(tunings), 1, sharex='all', sharey='all', figsize=(12, 8), squeeze=False)

try:
    with Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT) as ser:
        for i, tuning in enumerate(tunings):
            print(f"[TEST {i+1}/{len(tunings)}] {get_readable_name(tuning)}")
            filename = 'data/data' + get_tuning_name(tuning) + '.tsv'
            print(f"  -> Fichier de log : {filename}")

            if not isfile(filename):
                with open(filename, 'w') as f:
                    ser.reset_input_buffer()

                    set_tuning(ser, tuning)
                    _ = read_slip(ser)  # vider réponse éventuelle
                    ser.reset_input_buffer()

                    start(ser)

                    # Lire jusqu'à fin d'expérience
                    while True:
                        data = read_slip(ser)
                        if data is None:
                            break
                        f.write('\\t'.join(map(str, data)) + '\\n')

                print(f"  -> Données sauvegardées dans {filename}")
            else:
                print("  ⚡️ Données déjà présentes, on les réutilise")

            try:
                data = genfromtxt(filename, delimiter='\\t')
                axs[i][0].plot(data, linewidth=1)
            except Exception as e:
                print(f"  ! Warning: lecture échouée pour {filename} : {e}")

            axs[i][0].axhline(0, linewidth=0.5, color='k')
            axs[i][0].set_title(get_readable_name(tuning))

except SerialException as e:
    print(f"[ERROR] Serial open failed: {e}")
    sys.exit(1)

plt.tight_layout()
timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
plt.savefig(f"{img_basename}-{timestamp}.svg")
plt.savefig(f"{img_basename}-{timestamp}.png", dpi=300)
print(f"[INFO] Figures sauvegardées : {img_basename}-{timestamp}.svg & .png")
plt.show()
