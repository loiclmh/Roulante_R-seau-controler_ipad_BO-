from serial import Serial
import struct, time
SLIP_END=0xC0; SLIP_ESC=0xDB; SLIP_ESC_END=0xDC; SLIP_ESC_ESC=0xDD
def slip_enc(p: bytes) -> bytes:
    out=bytearray([SLIP_END])
    for b in p:
        if b==SLIP_END: out+=bytes([SLIP_ESC,SLIP_ESC_END])
        elif b==SLIP_ESC: out+=bytes([SLIP_ESC,SLIP_ESC_ESC])
        else: out.append(b)
    out.append(SLIP_END); return out

if __name__ == "__main__":
    with Serial("COM3", 1_000_000, timeout=0.5) as s:
        time.sleep(1.0)
        s.write(slip_enc(b's0'+struct.pack('<f', 8.0)))
        print("Envoye: s0")
