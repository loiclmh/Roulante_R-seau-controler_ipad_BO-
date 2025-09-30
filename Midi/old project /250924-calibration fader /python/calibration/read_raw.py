from serial import Serial
import time
SLIP_END=0xC0; SLIP_ESC=0xDB; SLIP_ESC_END=0xDC; SLIP_ESC_ESC=0xDD

def read_frames(port='COM3', baud=1_000_000):
    with Serial(port, baud, timeout=1) as ser:
        print("Ouvert:", ser.portstr)
        buf=bytearray(); frames=0
        t0 = time.time()
        while time.time() - t0 < 5.0:  # lit 5 secondes
            b = ser.read(1)
            if not b: continue
            b = b[0]
            if b==SLIP_END:
                if buf:
                    frames+=1
                    print(f"Frame #{frames} len={len(buf)}")
                    buf.clear()
            else:
                buf.append(b)
        print("Frames totales:", frames)

if __name__ == "__main__":
    read_frames()
