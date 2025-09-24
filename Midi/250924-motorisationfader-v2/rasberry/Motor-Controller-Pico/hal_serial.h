#pragma once
#include <Arduino.h>

namespace HALSerial {
  inline void begin() {
    Serial.begin(1000000);           // pico sdk CDC
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0) < 500) delay(10); // non bloquant
  }
  inline bool available() { return Serial.available() > 0; }
  inline int  read()      { return Serial.read(); }
  inline size_t write(uint8_t b) { return Serial.write(b); }
  inline size_t write(const uint8_t* buf, size_t n) { return Serial.write(buf, n); }
  inline void flush() { Serial.flush(); }
}