#pragma once
#include <Arduino.h>

// Un buffer par unité de compilation (ok pour header-only)
static String __comms_buf;

// --- API ---
inline void comms_begin() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 1500) { /* wait up to ~1.5s */ }
  Serial.println("READY");
}

inline bool comms_available_line() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') return true;
    __comms_buf += c;
  }
  return false;
}

inline String comms_take_line() {
  String out = __comms_buf;
  __comms_buf = "";
  return out;
}

// Helpers "clé=valeur"
inline void comms_kv(const String& k, const String& v) {
  Serial.print(k); Serial.print('='); Serial.println(v);
}
inline void comms_kv(const String& k, int v)   { comms_kv(k, String(v)); }
inline void comms_kv(const String& k, float v) { comms_kv(k, String(v, 3)); }