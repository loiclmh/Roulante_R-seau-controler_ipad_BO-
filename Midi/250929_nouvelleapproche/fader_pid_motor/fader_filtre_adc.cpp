#include <Arduino.h>
#include "fader_filtre_adc.h"
#include "display.h"
#include "debug.h"

// ====== Control Surface (obligatoire) ======
#include <Arduino_Helpers.h>
#include <AH/Hardware/FilteredAnalog.hpp>

// ===================== ÉTAT =====================
// Un filtre Control Surface par fader
static AH::FilteredAnalog<MY_ADC_BITS, FILTER_SHIFT, uint32_t>* gFaders[MAX_FADERS] = {nullptr};
uint16_t gFaderADC[MAX_FADERS] = {0}; // valeurs filtrées brutes 0..ADC_MAX
uint8_t  fader_idx = 0;               // fader/moteur à tester/envoyer (unique ici)

// ===================== UTILS =====================
static inline int clamp(int v, int lo, int hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

// ===================== SETUP/LOOP =====================
void setupADC() {
  static_assert(NUM_FADERS >= 1 && NUM_FADERS <= MAX_FADERS,
                "NUM_FADERS doit être entre 1 et MAX_FADERS");

  for (uint8_t i = 0; i < NUM_FADERS; ++i) {
    pinMode(FADER_PINS[i], INPUT);
    gFaders[i] = new AH::FilteredAnalog<MY_ADC_BITS, FILTER_SHIFT, uint32_t>(FADER_PINS[i]);
    gFaders[i]->update();
    int raw = (int)gFaders[i]->getValue();
    raw = clamp(raw, USABLE_MIN, USABLE_MAX);
    gFaderADC[i] = raw;
  }

  if (debug_fadermoniteur == 1) {
    Serial.println("READY");
  }
}

void loopfader(uint8_t i) {
  if (i >= NUM_FADERS) return;

  // 1) Met à jour le filtre + lit la valeur
  gFaders[i]->update();
  int filt_raw   = (int)gFaders[i]->getValue();  // valeur filtrée CS
  int raw_direct = analogRead(FADER_PINS[i]);    // lecture brute ADC (debug)

  if (debugOLED_fader == 1) {
    Serial.print("Fader"); Serial.print(i); Serial.print(": ");
    Serial.print("rawCS="); Serial.print(filt_raw);
    Serial.print(" rawADC="); Serial.print(raw_direct);
    Serial.print("  ");
  }

  // 2) Zone morte mécanique + normalisation
  filt_raw = clamp(filt_raw, USABLE_MIN, USABLE_MAX);
  filt_raw = map(filt_raw, USABLE_MIN, USABLE_MAX, 0, ADC_MAX);

  // Snap bas/haut
  if (filt_raw < snap_low)  filt_raw = 0;
  if (filt_raw > snap_high) filt_raw = ADC_MAX;

  // 3) Deadband logiciel
  if (abs(filt_raw - (int)gFaderADC[i]) < DEADBAND_ADC) {
    filt_raw = gFaderADC[i];
  }

  // 4) Stockage
  gFaderADC[i] = (uint16_t)filt_raw;

  if (debugOLED_fader == 1) {
    Serial.print("ADC="); Serial.println(gFaderADC[i]);
  }

  // (Optionnel) Affichage OLED non bloquant / ou seulement pour i==0
  if (debugOLED_fader == 1 && i == 0) {
    drawOLED(gFaderADC[0]);
    // évite delay ici si possible (préférer un timer ailleurs)
    // delay(LOOP_DELAY_MS);
  }
}