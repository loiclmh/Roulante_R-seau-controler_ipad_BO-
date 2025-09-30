#include <Arduino.h>
#include "display.h"
#include "fader_filtre_adc.h"


void setup() {
    setupOLED();
    analogReadResolution(MY_ADC_BITS);
    Serial.begin(115200);
    unsigned long t0 = millis();
    while (!Serial && (millis() - t0) < 1500) {}
    setupADC();
}


void loop() {
  loopfader();
}

