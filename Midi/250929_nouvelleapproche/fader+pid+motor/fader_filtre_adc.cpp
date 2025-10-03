

#include <Arduino.h>
#include "display.h"
#include "fader_filtre_adc.h"


// ====== Control Surface (obligatoire) ======
#include <Arduino_Helpers.h>
#include <AH/Hardware/FilteredAnalog.hpp>

// ===================== ÉTAT =====================
// Un filtre Control Surface par fader
static AH::FilteredAnalog<MY_ADC_BITS, FILTER_SHIFT, uint32_t>* gFaders[MAX_FADERS] = {nullptr};
static int gFaderADC[MAX_FADERS] = {0}; // valeurs filtrées brutes 0..4095

// ===================== UTILS =====================
static inline int clamp(int v, int lo, int hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

// ===================== SETUP/LOOP =====================
void setupADC() {
  static_assert(NUM_FADERS >= 1 && NUM_FADERS <= MAX_FADERS, "NUM_FADERS doit être entre 1 et MAX_FADERS");

  // Init faders avec filtre Control Surface
  for (uint8_t i = 0; i < NUM_FADERS; ++i) {
    pinMode(FADER_PINS[i], INPUT);
  gFaders[i] = new AH::FilteredAnalog<MY_ADC_BITS, FILTER_SHIFT, uint32_t>(FADER_PINS[i]);
  gFaders[i]->update();
  int raw = (int)gFaders[i]->getValue();
    raw = clamp(raw, USABLE_MIN, USABLE_MAX);
    gFaderADC[i] = raw;
  }
  if  (debug_fadermoniteur == 1) { // ne s'active que si debug_fader=1
    Serial.println("READY");
  }
}

void loopfader() {
  for (uint8_t i = 0; i < NUM_FADERS; ++i) {
    // 1. Met à jour le filtre Control Surface puis récupère la valeur
    gFaders[i]->update();
    int filt_raw = (int)gFaders[i]->getValue();
    int raw_direct = analogRead(FADER_PINS[i]);
    if (debugOLED_fader == 1) { // ne s'active que si debug_fader=1
      Serial.print("raw="); Serial.print(filt_raw); Serial.print(" ");
      Serial.print(" rawCS="); Serial.print(filt_raw);
      Serial.print(" rawADC="); Serial.print(raw_direct);
      }
    // 2. Application de la zone morte mécanique
    filt_raw = clamp(filt_raw, USABLE_MIN, USABLE_MAX);
    filt_raw = map(filt_raw, USABLE_MIN, USABLE_MAX, 0, ADC_MAX);
    if (filt_raw < snap_low) {
      filt_raw = 0;
    }
    if (filt_raw > snap_high) {
      filt_raw = 4095;
    }
    // --- Deadband logiciel pour stabiliser ---
    if (abs(filt_raw - gFaderADC[i]) < DEADBAND_ADC) {
      filt_raw = gFaderADC[i]; // conserve l'ancienne valeur si variation minime
    }
    // 3. Stockage dans gFaderADC[i]
    gFaderADC[i] = filt_raw;

    if (debugOLED_fader == 1) { // ne s'active que si debug_fader=1
      Serial.print("Fader"); Serial.print(i);
      Serial.print("_ADC=");
      Serial.println(gFaderADC[i]);
    }
  }
  
  if (debugOLED_fader == 1) { // ne s'active que si debug_fader=1
  // Affichage OLED du premier fader
  drawOLED(gFaderADC[0]);
  delay(LOOP_DELAY_MS);
  }
}
