#include <Arduino.h>
#include "debug.hpp"
#include "display.h"
#include "fader_filtre_adc.h"
#include "pid.h"
#include "has_serial.h"

#include "motor.h"
#include "bash_test.hpp"


constexpr uint8_t bash_test_pid = 0; // active ou non com scrypte python
//

void setup() {
    debugsetup();
    setupOLED();
    analogReadResolution(MY_ADC_BITS);

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

    // PID : utilise les valeurs Python si on est en mode python ET bash_test_pid==1
    const bool use_python_vals = (on_debug && on_debug_python && (bash_test_pid == 1));
    initial_PIDv(use_python_vals);

    // Test local (séquentiel ou parallèle selon bash_test.hpp)
    if (bash_test_mode == 1) {
        bashTestBegin();
    }
}

void loop() {
    // 1) I/O série pour Tuning.py (SLIP p/i/d/t/c/s)
    handleTuningIO();

    // 2) Si le mode test local est actif, avance la séquence (séquentiel ou parallèle selon ton bash_test.hpp)
    bashTestLoop();

    // 3) Rafraîchir les faders (mesure filtrée -> gFaderADC[])
    loopfader();

    // 4) Mettre à jour chaque voie PID + commande moteur
    for (uint8_t i = 0; i < NUM_MOTOR; ++i) {
        loopPID(i);  // calcule u et pilote moteur i
    }
}
