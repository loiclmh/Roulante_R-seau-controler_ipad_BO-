"""
Tuning_mac.py — script macOS robuste pour les essais PID (Pico)
- Auto-détecte /dev/tty.usbmodem*
- Envoie Kp Ki Kd fc (+ s0) au Pico via SLIP
- Sauvegarde data/*.tsv et results/*.png, *.svg
"""

# ---------- Imports & vérifs dépendances ----------
import sys, glob, os
from pathlib import Path
from datetime import datetime

try:
    import struct
    import numpy as np
    import matplotlib.pyplot as plt
    from numpy import genfromtxt
    from serial import Serial
    from serial.tools import list_ports
    from time import sleep
    from SLIP import read_slip, write_slip
except ModuleNotFoundError as e:
    print("[ERREUR] Module manquant:", e)
    print("Installe les dépendances avec :")
    print("  python3 -m pip install --user numpy matplotlib pyserial")
    sys.exit(1)

# ---------- Helpers manquants ----------
def autodetect_port() -> str | None:
    """Détecte automatiquement le port du Pico sur macOS (/dev/tty.usbmodem*)."""
    # 1) glob direct
    cands = sorted(glob.glob('/dev/tty.usbmodem*') + glob.glob('/dev/tty.usbserial*'))
    if cands:
        return cands[0]
    # 2) via pyserial
    try:
        for p in list_ports.comports():
            name = getattr(p, 'device', '') or ''
            if 'usbmodem' in name or 'usbserial' in name:
                return name
            # VID du RP2040 (Raspberry Pi)
            if getattr(p, "vid", None) == 0x2E8A:
                return name
    except Exception:
        pass
    return None

def get_tuning_name(tuning: tuple):
    """Nom compact pour fichiers: p.. i.. d.. c.."""
    kp, ki, kd, fc = tuning
    return f"p{kp}i{ki}d{kd}c{fc}"

def get_readable_name(tuning: tuple):
    """Nom lisible pour titres: Kp, Ki, Kd, fc formatés."""
    kp, ki, kd, fc = tuning
    return f"$K_p = {kp:.2f}$,  $K_i = {ki:.2f}$,  $K_d = {kd:.4f}$,  $f_c = {fc:.2f}$"

def set_tuning(ser, tuning: tuple):
    """Envoie Kp, Ki, Kd, fc en SLIP: 'p0','i0','d0','c0' + float32 LE."""
    kp, ki, kd, fc = tuning
    for setting, value in zip(('p','i','d','c'), (kp, ki, kd, fc)):
        msg = (setting + str(fader_idx)).encode() + struct.pack('<f', float(value))
        write_slip(ser, msg)
        sleep(0.01)

def start(ser):
    """Démarre l'expérience: 's0' + speed_div (float32 LE)."""
    msg = b's0' + struct.pack('<f', float(speed_div))
    write_slip(ser, msg)

# ---------- Réglages ----------
BAUDRATE = 1_000_000
TIMEOUT  = 0.5

# Base choisie (best actuel)
BASE_KP = 2.80
BASE_KI = 0.67
BASE_KD = 0.0027
BASE_FC = 60

# Sélection du balayage fin : 'KD', 'KI', 'KP'
SWEEP   = 'KP'        # ← change ici pour passer KD -> KI -> KP
STEP_KD = 0.0002      # pas fin demandé
STEP_KI = 0.02
STEP_KP = 0.02

# Construit 4 tests par passe autour de la base
if SWEEP == 'KD':
    kd_vals = [BASE_KD - STEP_KD, BASE_KD, BASE_KD + STEP_KD, BASE_KD + 2*STEP_KD]
    tunings = [(BASE_KP, BASE_KI, kd, BASE_FC) for kd in kd_vals]
elif SWEEP == 'KI':
    ki_vals = [BASE_KI - 2*STEP_KI, BASE_KI - STEP_KI, BASE_KI, BASE_KI + STEP_KI]
    tunings = [(BASE_KP, ki, BASE_KD, BASE_FC) for ki in ki_vals]
elif SWEEP == 'KP':
    kp_vals = [BASE_KP - 2*STEP_KP, BASE_KP - STEP_KP, BASE_KP, BASE_KP + STEP_KP]
    tunings = [(kp, BASE_KI, BASE_KD, BASE_FC) for kp in kp_vals]
else:
    raise ValueError("SWEEP doit être 'KD', 'KI' ou 'KP'")

speed_div = 8.0
fader_idx = 0
img_basename = 'tuning_result'

# Répéter chaque essai pour fiabiliser et rejeter la pire répétition
N_REPEATS = 5
N_KEEP    = 4    # on moyenne les 4 meilleures

# --------- Helpers d'alignement (détecter la montée de consigne) ----------
def _edge_index(ref: np.ndarray) -> int:
  """Retourne l'index du premier fort changement de ref (début de l'essai)."""
  if ref is None or len(ref) < 3:
    return 0
  d = np.diff(ref.astype(float))
  amp = float(np.max(ref) - np.min(ref) + 1e-9)
  thr = 0.02 * amp  # 2% d'amplitude
  idx = np.argmax(np.abs(d) > thr)
  # si aucun dépassement, idx sera 0 mais c'est ok
  return int(idx)

def _align_and_trim(ref: np.ndarray, pos: np.ndarray):
  """Coupe au début de la rampe (edge) et retourne (ref_aligned, pos_aligned)."""
  i0 = _edge_index(ref)
  ref_a = ref[i0:].astype(float)
  pos_a = pos[i0:].astype(float)
  return ref_a, pos_a

def _rmse(ref: np.ndarray, pos: np.ndarray) -> float:
  """Erreur quadratique moyenne entre pos et ref."""
  if len(ref) == 0 or len(pos) == 0:
    return float('inf')
  e = (pos.astype(float) - ref.astype(float))
  return float(np.sqrt(np.mean(e*e)))

# ---------- Dossiers & figure ----------
root = Path(__file__).parent
os.chdir(root)
DATA_DIR    = root / 'data'
RESULTS_DIR = root / 'results'
DATA_DIR.mkdir(exist_ok=True)
RESULTS_DIR.mkdir(exist_ok=True)

fig, axs = plt.subplots(len(tunings), 1, sharex='all', sharey='all',
                        figsize=(12, 8), squeeze=False)

# ---------- Port série ----------
SERIAL_PORT = autodetect_port()
if SERIAL_PORT is None:
    print("[ERREUR] Aucun port usbmodem trouvé.")
    print("• Débranche/rebranche le Pico")
    print("• Ferme le Moniteur série Arduino")
    print("• Vérifie:  ls /dev/tty.usbmodem*")
    sys.exit(1)
print(f"[INFO] Port série utilisé : {SERIAL_PORT}")

# ---------- Run ----------
with Serial(SERIAL_PORT, BAUDRATE, timeout=TIMEOUT) as ser:
    for i, tuning in enumerate(tunings):
        print("==>", get_readable_name(tuning))

        # Collecte des répétitions alignées
        reps_ref = []
        reps_pos = []

        for r in range(N_REPEATS):
            filename = DATA_DIR / f"data{get_tuning_name(tuning)}_r{r+1}.tsv"

            with open(str(filename), 'w') as f:
                ser.reset_input_buffer()
                # Envoie PID
                set_tuning(ser, tuning)
                read_slip(ser)              # purge éventuelle
                ser.reset_input_buffer()
                # Démarre
                start(ser)

                # Lis jusqu'à fin d'essai (None)
                while True:
                    data = read_slip(ser)
                    if data is None:
                        break
                    f.write('\t'.join(map(str, data)) + '\n')

            # Charge et aligne cette répétition
            data = genfromtxt(filename, delimiter='\t')
            try:
                n = getattr(data, 'shape', (0,))[0]
            except Exception:
                n = 0
            if n == 0:
                print(f"[WARN] Fichier vide: {filename} — saut de la répétition {r+1}.")
                continue

            try:
                ref = np.array(data[:, 0])
                pos = np.array(data[:, 1])
            except Exception:
                # fallback 1D
                ref = np.array(data[0])
                pos = np.array(data[1])

            ref_a, pos_a = _align_and_trim(ref, pos)
            reps_ref.append(ref_a)
            reps_pos.append(pos_a)

        if len(reps_ref) == 0:
            print("[WARN] Aucune donnée valide pour ce tuning — pas de tracé.")
            continue

        # Sélectionne les N_KEEP meilleures répétitions (plus faible RMSE pos vs ref)
        scores = []
        for rf, ps in zip(reps_ref, reps_pos):
            # tronque à la même longueur temporairement pour comparer
            L = min(len(rf), len(ps))
            scores.append(_rmse(rf[:L], ps[:L]))
        # indices triés par RMSE croissant
        keep_idx = np.argsort(np.array(scores))[:min(N_KEEP, len(scores))]
        reps_ref = [reps_ref[i] for i in keep_idx]
        reps_pos = [reps_pos[i] for i in keep_idx]

        # Tronque toutes les répétitions à la même longueur minimale
        min_len = min(len(rf) for rf in reps_ref)
        reps_ref = [rf[:min_len] for rf in reps_ref]
        reps_pos = [ps[:min_len] for ps in reps_pos]

        # Moyennes
        ref_avg = np.mean(np.stack(reps_ref, axis=0), axis=0)
        pos_avg = np.mean(np.stack(reps_pos, axis=0), axis=0)

        # Tracé (position réelle vs consigne) — on supprime la courbe moteur 'u'
        ax = axs[i][0]
        ax.plot(ref_avg, linewidth=1.2, label='ref (moy)', color='tab:blue')
        ax.plot(pos_avg, linewidth=1.2, label='pos (moy)', color='tab:orange')
        ax.axhline(0, linewidth=0.5, color='k')
        ax.set_title(get_readable_name(tuning) + "  — moy. de {} rép.".format(len(reps_ref)))
        ax.set_ylabel('ref / pos')
        ax.legend(loc='upper left', fontsize=8, frameon=False)
        ax.grid(True, which='both', linestyle=':', linewidth=0.5, alpha=0.5)

# ---------- Sauvegarde figure ----------
timestamp = datetime.now().strftime('%Y%m%d-%H%M%S')
svg_path = RESULTS_DIR / f"{img_basename}-{timestamp}.svg"
png_path = RESULTS_DIR / f"{img_basename}-{timestamp}.png"
plt.tight_layout()
plt.savefig(str(svg_path))
plt.savefig(str(png_path), dpi=300)
print("Figures sauvegardées :", svg_path, png_path)

plt.show()