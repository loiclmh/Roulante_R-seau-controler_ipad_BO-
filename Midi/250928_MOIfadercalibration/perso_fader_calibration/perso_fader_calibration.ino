#include <Arduino.h>
#include "pins.h"
#include "Bash_test.h"
#include "display.h"
#include "comms.hpp"
#include "calibration.h"
#include "test1_call_ADC.hpp"

// Forward decls for tests
void run_test1();
void run_test2();
void run_test3();

// Protos console/calibration série
void serialCalibBegin();
void serialCalibLoop();

void setup() {
  // 1) Comms d'abord (utile si tu veux voir les logs de boot)
  comms_begin();            // comms.hpp

  // 2) Calibration (active min/max selon TEST)
  calibInit();

  // 3) Périphériques
  analogReadResolution(12); // RP2040: 12 bits (0..4095)
  display_begin();

  // 4) Console série de calibration à chaud (optionnel)
  serialCalibBegin();

  // 5) Feedback visuel & log
  display_status("PicoFaderTest", "Fader + OLED", String("#") + TEST);
}

void loop() {

switch(TEST) {
  case 1: run_test1(); break;
  case 2: run_test2(); break;
  case 3: run_test3(); break;
  default:
    display_status("TEST inconnu", String("#") + TEST);
    delay(300);
  }


  
}



