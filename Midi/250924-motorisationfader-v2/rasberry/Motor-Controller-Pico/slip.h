#pragma once
#include <Arduino.h>
#include "hal_serial.h"

// SLIP constants
static constexpr uint8_t SLIP_END     = 0xC0;
static constexpr uint8_t SLIP_ESC     = 0xDB;
static constexpr uint8_t SLIP_ESC_END = 0xDC;
static constexpr uint8_t SLIP_ESC_ESC = 0xDD;

// Encode (host ← Pico)
inline void slipWriteByte(uint8_t b) {
  if (b == SLIP_END)      { HALSerial::write(SLIP_ESC); HALSerial::write(SLIP_ESC_END); }
  else if (b == SLIP_ESC) { HALSerial::write(SLIP_ESC); HALSerial::write(SLIP_ESC_ESC); }
  else                    { HALSerial::write(b); }
}
inline void slipWritePacket(const uint8_t* data, size_t len) {
  HALSerial::write(SLIP_END);
  for (size_t i=0;i<len;i++) slipWriteByte(data[i]);
  HALSerial::write(SLIP_END);
}

// Decode
struct SlipDecoder {
  uint8_t buf[128];
  size_t  len = 0;
  bool    esc = false;

  // retourne true quand un paquet complet est reçu (dans buf,len)
  bool poll() {
    while (HALSerial::available()) {
      int r = HALSerial::read();
      if (r < 0) break;
      uint8_t b = (uint8_t)r;
      if (b == SLIP_END) {
        if (len > 0) {
          // paquet prêt : NE PAS vider ici (le caller consommera buf/len)
          return true;
        } else {
          // séparateur/keep-alive
          len = 0; esc = false;
        }
      } else if (b == SLIP_ESC) {
        esc = true;
      } else {
        if (esc) {
          if (b == SLIP_ESC_END)      b = SLIP_END;
          else if (b == SLIP_ESC_ESC) b = SLIP_ESC;
          esc = false;
        }
        if (len < sizeof(buf)) buf[len++] = b;
      }
    }
    return false;
  }
};
