#include "pins.h"
#include "Bash_test.h"
#include "display.h"
#include "comms.h"

// Forward decls for tests
void run_test1();
void run_test2();
void run_test3();

void setup() {
  comms_begin();
  display_begin();
  analogReadResolution(12); // RP2040 supports 12-bit ADC
  display_status("PicoFaderTest", "Fader + OLED", String("#") + TEST);
  Serial.print("TEST="); Serial.println(TEST);
}

void loop() {
  switch (TEST) {
    case 1: run_test1(); break;
    case 2: run_test2(); break;
    case 3: run_test3(); break;
    default:
      display_status("TEST inconnu", String("#") + TEST);
      delay(300);
  }
}
