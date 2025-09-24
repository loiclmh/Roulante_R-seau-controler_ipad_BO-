#pragma once
#include <Arduino.h>

namespace Display {
  void begin();                        // init OLED
  void showMessage(const char* text);  // texte simple
  void showCounter(int value);         // compteur pour debug
}
