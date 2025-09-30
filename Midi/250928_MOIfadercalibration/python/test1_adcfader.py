# Le test 1 permet de tester des sensibiliter et des zone morte du fader
#


import time, sys
import serial

PORT = "/dev/tty.usbmodem101"
BAUD = 115200

# Matrice de paramètres: (index, min, max, ns, hyst)
MATRIX = [
    (0, 40, 4020, 150, 25),
    # (1, 50, 4000, 128, 20),  # dé-commente si tu as un fader 2
]

def send(ser, line):
    ser.write((line + "\n").encode("utf-8"))
    ser.flush()
    print(">>", line)
    time.sleep(0.02)
    while ser.in_waiting:
        print("<<", ser.readline().decode("utf-8", "ignore").strip())

def main():
    ser = serial.Serial(PORT, BAUD, timeout=0.2)
    time.sleep(1.0)  # temps d'ouverture

    # Réveil & sync
    send(ser, "START")
    for (idx, mn, mx, ns, hy) in MATRIX:
        send(ser, f"SET F {idx} ALL {mn} {mx} {ns} {hy}")
        send(ser, f"GET F {idx}")

    # Lancer le test
    send(ser, "RUN")

    # Lire les logs pendant le test
    t0 = time.time()
    while time.time() - t0 < 5:
        if ser.in_waiting:
            print("<<", ser.readline().decode("utf-8", "ignore").strip())
        else:
            time.sleep(0.05)

    ser.close()

if __name__ == "__main__":
    main()