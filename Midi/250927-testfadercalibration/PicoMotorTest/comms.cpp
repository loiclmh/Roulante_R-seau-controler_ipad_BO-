#include <Arduino.h>
static String buf;
static bool hasLine = false;

bool comms_available_line() {
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c=='\r' || c=='\n') {
      if (buf.length()) { hasLine = true; return true; }
    } else {
      buf += c;
    }
  }
  return hasLine;
}

String comms_take_line() {
  hasLine = false;
  String out = buf;
  buf = "";
  return out;
}

void comms_begin() {
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 1500) {}
  Serial.println("READY");
}
