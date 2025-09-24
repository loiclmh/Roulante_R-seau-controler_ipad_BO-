#pragma once
#include <Arduino.h>

namespace Display {
  void begin();
  void showSplash(const char* text);
  void showFaderAndMotor(uint16_t y_smooth_10b, uint16_t y_raw_10b, int16_t u_cmd);
}
