# SLIP.py – mini-wrapper sans dépendances
# Fournit: write_slip(ser, payload: bytes), read_slip(ser) -> list|bytes|None
# Encodage SLIP minimal: encode bytes et termine par END (0xC0).
# read_slip lit jusqu’à END et tente de parser en champs tabulés.

END = 0xC0
ESC = 0xDB
ESC_END = 0xDC
ESC_ESC = 0xDD

def _encode_slip(payload: bytes) -> bytes:
    out = bytearray()
    for b in payload:
        if b == END:
            out += bytes([ESC, ESC_END])
        elif b == ESC:
            out += bytes([ESC, ESC_ESC])
        else:
            out.append(b)
    out.append(END)
    return bytes(out)

def _decode_slip(buf: bytes) -> bytes:
    out = bytearray()
    i = 0
    n = len(buf)
    while i < n:
        b = buf[i]
        if b == ESC:
            i += 1
            if i >= n: break
            nb = buf[i]
            if     nb == ESC_END: out.append(END)
            elif   nb == ESC_ESC: out.append(ESC)
            else:                 out.append(nb)  # tolérant
        elif b != END:
            out.append(b)
        i += 1
    return bytes(out)

def write_slip(ser, payload: bytes):
    """Envoie un payload binaire encodé en SLIP sur le port série."""
    ser.write(_encode_slip(payload))

def read_slip(ser):
    """Lit une trame SLIP. Retourne:
       - liste de champs parsés (si tab-séparés)
       - sinon bytes décodés
       - None si timeout (pas de trame)
    """
    buf = bytearray()
    while True:
        b = ser.read(1)
        if not b:
            # Timeout sans rien lire
            return None if not buf else []
        if b[0] == END:
            if not buf:
                # END seul (délimiteur vide), on continue à lire
                continue
            decoded = _decode_slip(bytes(buf))
            # Essaie de parser "a\tb\tc" → liste de nombres
            try:
                txt = decoded.decode('utf-8', errors='ignore').strip()
                if '\t' in txt:
                    parts = txt.split('\t')
                    def parse(x):
                        try:
                            return float(x) if ('.' in x or 'e' in x or 'E' in x) else int(x)
                        except Exception:
                            return x
                    return [parse(p) for p in parts]
            except Exception:
                pass
            return decoded
        else:
            buf.append(b[0])
            