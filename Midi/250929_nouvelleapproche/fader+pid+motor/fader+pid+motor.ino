#include <Arduino.h>
#include "display.h"
#include "fader_filtre_adc.h"
#include "pid.h"
#include "has_serial.h"
#include "motor.h"


constexpr uint8_t bash_test_pid = 1; // active ou non com scrypte python
//

void setup() {
    setupOLED();
    analogReadResolution(MY_ADC_BITS);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && (millis() - t0) < 1500) {}
    setupADC();
}


void loop() {
if 
  loopfader();
}

