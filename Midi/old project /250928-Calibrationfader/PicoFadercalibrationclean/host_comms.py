"""
Exemple minimal de script Python pour parler avec la carte via USB-Série.

Dépendance: pip install pyserial
Usage:
  python tools/host_comms.py
"""
import time, sys
import serial

def find_port(preferred=None):
    if preferred:
        return preferred
    # Laissez l'utilisateur préciser le port en argument si besoin
    return None

def main(port=None, baud=115200):
    if port is None and len(sys.argv) > 1:
        port = sys.argv[1]
    if port is None:
        print("⚠️  Donnez le port série en argument, ex: python tools/host_comms.py /dev/tty.usbmodem101")
        return
    ser = serial.Serial(port, baudrate=baud, timeout=0.2)
    time.sleep(0.5)
    # Flush initial
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    # Attendre "READY"
    t0 = time.time()
    got = ""
    while time.time() - t0 < 2.0:
        if ser.in_waiting:
            got += ser.read(ser.in_waiting).decode(errors="ignore")
            if "READY" in got:
                break
        time.sleep(0.05)
    print("MCU says:", got.strip() or "(no banner)")

    # Ping
    ser.write(b"PING\n")
    time.sleep(0.05)
    if ser.in_waiting:
        print("RX:", ser.read(ser.in_waiting).decode(errors="ignore").strip())

    # Lecture de quelques lignes RAW/OUT8
    print("Streaming 20 lines...")
    n = 0
    while n < 20:
        line = ser.readline().decode(errors="ignore").strip()
        if not line:
            continue
        print(line)
        n += 1

    ser.close()

if __name__ == "__main__":
    main()
