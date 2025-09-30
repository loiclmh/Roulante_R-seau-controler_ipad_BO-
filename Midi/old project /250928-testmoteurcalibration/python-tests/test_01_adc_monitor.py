#!/usr/bin/env python3
# Test 01 — Segmented calibration (RAW only)
# 16 segments par défaut, 3 s de déplacement (OLED plein blanc), 10 s de mesure au repos.

import sys, glob, time
from pathlib import Path
from datetime import datetime
import serial
import numpy as np
import struct
import json
import matplotlib.pyplot as plt

SEG_DEFAULT = 16          # nombre de segments
MEAS_SEC_DEFAULT = 10.0   # durée mesure par segment
MOVE_SEC_DEFAULT = 3.0    # durée fenêtre déplacement
HZ_DEFAULT = 50

# deadzone = k * MAD_local (autour du centre par segment)
K_DEFAULT = 3.0
CLIP_COUNTS = 6           # fenêtre pour MAD autour du centre
MARGIN_PCT = 0.02         # +2% de marge sur la DZ calculée (relative)


def autodetect_port():
    cands = sorted(glob.glob('/dev/tty.usbmodem*')) or sorted(glob.glob('/dev/ttyACM*'))
    return cands[0] if cands else None


def open_serial(port=None, baud=115200):
    port = port or autodetect_port()
    if not port:
        print('No serial port found. Use --port /dev/tty.usbmodemXXXX', file=sys.stderr)
        sys.exit(2)
    ser = serial.Serial(port, baud, timeout=0.2)
    time.sleep(0.3)
    ser.reset_input_buffer()
    return ser


def send_line(ser, s: str):
    ser.write((s + "\n").encode('utf-8'))
    time.sleep(0.02)


def read_raw_once(ser):
    """Lit exactement 2 octets LE après avoir envoyé READRAW."""
    send_line(ser, 'READRAW')
    data = ser.read(2)
    if len(data) == 2:
        return struct.unpack('<H', data)[0]
    return None


def acquire_segment(ser, seg_idx, seconds, hz):
    """Acquiert (t,val) pendant `seconds` pour un segment donné."""
    rows = []
    dt = 1.0/float(hz)
    t0 = time.time()
    while time.time() - t0 < seconds:
        val = read_raw_once(ser)
        if val is not None:
            ts = time.time() - t0
            rows.append((ts, int(val)))
            print(f"seg {seg_idx:02d}  {ts:6.3f}\t{val}")
        time.sleep(dt)
    return rows


def robust_center_and_dz(vals, k=K_DEFAULT):
    """Centre = mode entier; DZ = k * MAD_local (clip ±CLIP_COUNTS)."""
    if len(vals) == 0:
        return 0, 0
    arr = np.asarray(vals, dtype=int)
    off = int(arr.min())
    counts = np.bincount(arr - off)
    center = int(off + np.argmax(counts))
    # MAD local autour du centre
    arrf = arr.astype(float)
    inliers = arrf[(arrf >= center-CLIP_COUNTS) & (arrf <= center+CLIP_COUNTS)]
    base = inliers if inliers.size >= 10 else arrf
    mad = float(np.median(np.abs(base - np.median(base)))) if base.size else 0.0
    spread = (1.4826 * mad) if mad > 0 else (float(np.std(base)) if base.size else 0.0)
    dz = int(max(1, round(k * spread)))
    # marge +2% relative
    dz = int(np.ceil(dz * (1.0 + MARGIN_PCT)))
    return center, dz


def plot_segment(ts_tag, mode, seg_idx, rows, center, dz):
    out_dir = Path('results_test_01')
    out_dir.mkdir(parents=True, exist_ok=True)
    tag = f"{ts_tag}_test01_{mode}_seg{seg_idx:02d}"
    # Courbe temporelle
    if rows:
        t = np.array([r[0] for r in rows])
        v = np.array([r[1] for r in rows])
        plt.figure()
        plt.plot(t, v)
        plt.xlabel('time [s]')
        plt.ylabel('ADC raw')
        plt.title(f'Segment {seg_idx:02d} — center={center}, dz=±{dz}')
        plt.grid(True, linestyle=':')
        plt.axhline(center, linestyle='--')
        plt.axhline(center + dz, linestyle=':')
        plt.axhline(center - dz, linestyle=':')
        png = out_dir / f"{tag}_adc.png"
        plt.tight_layout(); plt.savefig(png, dpi=160); plt.close()
        print('Saved', png)
        # Histogramme
        plt.figure()
        plt.hist(v, bins=50)
        plt.axvline(center, linestyle='--')
        plt.axvline(center + dz, linestyle=':')
        plt.axvline(center - dz, linestyle=':')
        plt.xlabel('ADC raw'); plt.ylabel('count')
        plt.title(f'Segment {seg_idx:02d} — histogram')
        png2 = out_dir / f"{tag}_hist.png"
        plt.tight_layout(); plt.savefig(png2, dpi=160); plt.close()
        print('Saved', png2)


def main(port=None, seconds_total=None, hz=HZ_DEFAULT, mode="raw", segments=SEG_DEFAULT, meas_sec=MEAS_SEC_DEFAULT, move_sec=MOVE_SEC_DEFAULT, k=K_DEFAULT):
    ser = open_serial(port)
    ts_tag = datetime.now().strftime('%y%m%d%H%M')
    tag_base = f'{ts_tag}_test01_{mode}'
    print('Connected to', ser.port)

    # Découpe en segments uniformes 0..4095
    edges = np.linspace(0, 4096, segments+1, dtype=int)

    centers = []
    deadzones = []

    all_rows = []  # pour garder (seg_idx, t, v)

    for i in range(segments):
        lo, hi = int(edges[i]), int(edges[i+1]-1)
        print(f"\n=== Segment {i+1}/{segments}  [{lo}..{hi}] ===")

        # Fenêtre mouvement: allumer l'écran (cue ON), l'utilisateur bouge le fader vers la zone suivante
        send_line(ser, 'CUE ON')
        print(f"MOVE NOW for {move_sec}s (OLED ON)")
        time.sleep(move_sec)
        send_line(ser, 'CUE OFF')
        print("HOLD STILL (measuring)...")

        # Acquisition
        rows = acquire_segment(ser, i, meas_sec, hz)
        all_rows.append((i, rows))

        # Analyse du segment
        vals = [v for _, v in rows]
        c, dz = robust_center_and_dz(vals, k=k)
        centers.append(int(c))
        deadzones.append(int(dz))

        # Sauvegarde data segment
        data_dir = Path('data'); data_dir.mkdir(parents=True, exist_ok=True)
        seg_path = data_dir / f"{tag_base}_seg{i:02d}.tsv"
        with open(seg_path, 'w') as f:
            for t,v in rows:
                f.write(f"{t:.6f}\t{v}\n")
        print('Saved', seg_path)

        # Graphes par segment
        plot_segment(ts_tag, mode, i, rows, c, dz)

    # Sauvegarde profil calibration
    prof = {
        'ts': ts_tag,
        'mode': mode,
        'segments': segments,
        'edges': edges.tolist(),
        'centers': centers,
        'deadzones': deadzones,
        'k': k,
        'margin_pct': MARGIN_PCT
    }
    out_prof = Path('results_test_01')/f"{tag_base}_calibration_profile.json"
    out_prof.parent.mkdir(parents=True, exist_ok=True)
    with open(out_prof, 'w') as f:
        json.dump(prof, f, indent=2)
    print('Saved', out_prof)

    # Génération d'un calibration.h minimal (16 segments par défaut)
    cal_h = Path('../PicoMotorTest/calibration.h')
    cal_h.parent.mkdir(parents=True, exist_ok=True)
    with open(cal_h, 'w') as f:
        f.write('// Auto-generated by test_01 (segmented calibration)\n')
        f.write(f'// {ts_tag}\n\n')
        f.write(f'constexpr int FADER_SEGMENTS = {segments};\n')
        f.write('constexpr int FADER_EDGES[FADER_SEGMENTS+1] = {')
        f.write(','.join(str(int(x)) for x in edges.tolist()))
        f.write('};\n')
        f.write('\nconstexpr int FADER_CENTER_LUT[FADER_SEGMENTS] = {')
        f.write(','.join(str(int(x)) for x in centers))
        f.write('};\n')
        f.write('\nconstexpr int FADER_DEADZONE_LUT[FADER_SEGMENTS] = {')
        f.write(','.join(str(int(x)) for x in deadzones))
        f.write('};\n')
    print('Wrote ../PicoMotorTest/calibration.h')


if __name__ == '__main__':
    # CLI minimal
    port = None
    if '--port' in sys.argv:
        port = sys.argv[sys.argv.index('--port')+1]
    hz = HZ_DEFAULT
    if '--hz' in sys.argv:
        hz = float(sys.argv[sys.argv.index('--hz')+1])
    segments = SEG_DEFAULT
    if '--segments' in sys.argv:
        segments = int(sys.argv[sys.argv.index('--segments')+1])
    meas_sec = MEAS_SEC_DEFAULT
    if '--meas-sec' in sys.argv:
        meas_sec = float(sys.argv[sys.argv.index('--meas-sec')+1])
    move_sec = MOVE_SEC_DEFAULT
    if '--move-sec' in sys.argv:
        move_sec = float(sys.argv[sys.argv.index('--move-sec')+1])
    k = K_DEFAULT
    if '--k' in sys.argv:
        k = float(sys.argv[sys.argv.index('--k')+1])

    main(port=port, hz=hz, mode='raw', segments=segments, meas_sec=meas_sec, move_sec=move_sec, k=k)
