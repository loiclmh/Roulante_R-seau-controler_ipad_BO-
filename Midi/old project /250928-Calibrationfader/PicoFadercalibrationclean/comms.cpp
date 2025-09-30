#include <Arduino.h>
static String buf;

void comms_begin() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 1500) { /* wait up to ~1.5s */ }
  Serial.println("READY");
}

bool comms_available_line() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\r') continue;
    if (c == '\n') return true;
    buf += c;
  }
  return false;
}

String comms_take_line() {
  String out = buf;
  buf = "";
  return out;
}
