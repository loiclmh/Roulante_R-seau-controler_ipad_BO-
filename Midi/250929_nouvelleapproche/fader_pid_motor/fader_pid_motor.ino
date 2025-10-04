#include <Arduino.h>
#include "display.h"
#include "fader_filtre_adc.h"
#include "pid.h"
#include "motor.h"
#include "bash_test_LOCAL.hpp"
#include "bash_test_python.hpp"
#include "debug.h"


// === Variables pour communication Python ===
float t_sec = 0.0f;       // temps en secondes (sera mis à jour dans loop)
//

void setup() {
    debugsetup();
    setupOLED();
    analogReadResolution(MY_ADC_BITS);
    for (uint8_t i = 0; i < NUM_MOTOR; ++i) {
        setPosition[i] = 0; // par défaut au minimum
        Dirmotor[i] = 0;   // par défaut moteur à l'arrêt
    }

    // ------------------------------
    // Sélection du mode série (un seul)
    // ------------------------------
    if (on_debug) {
        if (on_debug_python) {
            // Tuning.py → haut débit
            Serial.begin(1000000);
            unsigned long t0 = millis();
            while (!Serial && (millis() - t0) < 1500) {}
        } else if (on_debug_monitorarduino) {
            // Moniteur Arduino
            Serial.begin(115200);
            delay(10);
        }
    }
    // ADC & filtres faders
    setupADC();
    setupmotor();

    // PID : utilise les valeurs Python si on est en mode python ET bash_test_pid==1
    const bool use_python_vals = (on_debug && on_debug_python && (bash_test_pid == 1));
    initial_PIDv(use_python_vals);

    // Test local (séquentiel ou parallèle selon bash_test.hpp)
    if (bash_test_mode == 1) {
        bashTestBegin();
    }
}

void loop() {
    // mettre à jour le temps (millis() → secondes)
    t_sec = millis() / 1000.0f;

    // Boucle PID pour tous les faders
  if (bash_test_mode == 2) {       // Python
    uint8_t i = fader_idx;
    loopfader(i);
    loopPID(i);
    loopmotor(i);
  } else {                         // normal
    for (uint8_t i = 0; i < NUM_MOTOR; ++i) {
      loopfader(i);
      loopPID(i);
      loopmotor(i);
    }
  }



    // --- Bash test local ---
    if (bash_test_pid == 0) return; // pas de com Python
    if (bash_test_mode == 1) {
        loop_test_bash_local();  // ton test local
    }

    // --- Communication Python ---
    if (on_debug && on_debug_python) {
        tuningHandle();
        tuningSendSample(fader_idx, t_sec, setPosition[fader_idx], gFaderADC[fader_idx], Dirmotor[fader_idx]);
    }
}




