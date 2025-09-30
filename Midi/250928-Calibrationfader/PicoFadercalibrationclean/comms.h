#pragma once
#include <Arduino.h>

// Start Serial and announce when ready
void comms_begin();

// Non-blocking read of a full text line (ending with '\n'); returns true if a line is available.
bool comms_available_line();
String comms_take_line();

// Send a key=value pair over Serial on a single line for easy parsing from Python.
inline void comms_kv(const String& k, const String& v) {
  Serial.print(k); Serial.print('='); Serial.println(v);
}
inline void comms_kv(const String& k, int v)   { comms_kv(k, String(v)); }
inline void comms_kv(const String& k, float v) { comms_kv(k, String(v, 3)); }
