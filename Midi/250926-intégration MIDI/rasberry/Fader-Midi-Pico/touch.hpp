#pragma once
#include <Arduino.h>

// Essaie de récupérer la pin depuis pins.h si dispo, sinon défaut GP2
#if __has_include("pins.h")
  #include "pins.h"
#endif
#ifndef PIN_TOUCH_SENSE
  #define PIN_TOUCH_SENSE 2
#endif

/*
  Détection tactile capacitive (RC) sur PIN_TOUCH_SENSE

  Câblage requis :
    - 1 MΩ entre PIN_TOUCH_SENSE et 3V3
    - (optionnel) 22–100 nF entre PIN_TOUCH_SENSE et GND
    - Pad tactile relié au même nœud que la pin (même colonne breadboard)

  Principe :
    1) On décharge la pin (sortie LOW quelques µs)
    2) On libère la pin (entrée), elle remonte via la 1 MΩ
    3) On mesure le temps jusqu’à HIGH ; le doigt ↑ capacité → temps ↑
*/

// ================== Paramètres ajustables ==================
namespace TouchCfg {
  static const uint16_t DISCHARGE_US   = 20;     // force une vraie décharge
  static const uint16_t TIMEOUT_US     = 5000;   // garde-fou si ça ne monte jamais
  static const float    BASELINE_ALPHA = 0.002f; // EMA lente (drift)
  static const uint16_t MIN_BASELINE   = 15;     // évite baseline nulle
  static const uint16_t DELTA_TOUCH    = 40;     // seuil +µs pour "touch"
  static const uint16_t DELTA_RELEASE  = 25;     // seuil +µs pour "release"
}

// ================== État interne ==================
namespace TouchCap {
  static uint16_t baseline_us = 80;
  static bool     touched     = false;

  // Mesure brute du temps de montée (µs)
  static inline uint16_t measure_us() {
    // 1) Décharge : sortie LOW
    pinMode(PIN_TOUCH_SENSE, OUTPUT);
    digitalWrite(PIN_TOUCH_SENSE, LOW);
    delayMicroseconds(TouchCfg::DISCHARGE_US);

    // 2) Libère la pin (entrée haute impédance)
    pinMode(PIN_TOUCH_SENSE, INPUT);

    // 3) Mesure du temps jusqu'à HIGH
    const uint32_t t0 = micros();
    while ((uint32_t)(micros() - t0) < TouchCfg::TIMEOUT_US) {
      if (digitalRead(PIN_TOUCH_SENSE)) {
        return (uint16_t)(micros() - t0);
      }
    }
    // Timeout → valeur élevée, assimilée à "touch fort"
    return TouchCfg::TIMEOUT_US;
  }

  static inline void begin() {
    // Amorcer la baseline par moyenne de quelques mesures (doigt éloigné)
    uint32_t acc = 0;
    const uint8_t N = 40;
    for (uint8_t i = 0; i < N; ++i) {
      acc += measure_us();
      delay(2);
    }
    baseline_us = (uint16_t)max<uint32_t>(acc / N, TouchCfg::MIN_BASELINE);

    // Petite fenêtre de calibrage (ne pas toucher)
    delay(1500);
    touched = false;
  }

  static inline bool isTouched() {
    const uint16_t t = measure_us();

    // Mise à jour lente de la baseline (EMA)
    baseline_us = (uint16_t)((1.0f - TouchCfg::BASELINE_ALPHA) * baseline_us
                           +  TouchCfg::BASELINE_ALPHA * t);

    // Hystérésis
    const uint16_t th_on  = (uint16_t)(baseline_us + TouchCfg::DELTA_TOUCH);
    const uint16_t th_off = (uint16_t)(baseline_us + TouchCfg::DELTA_RELEASE);

    if (!touched) {
      if (t >= th_on) touched = true;
    } else {
      if (t <= th_off) touched = false;
    }
    return touched;
  }

  // Helpers debug (optionnels)
  static inline uint16_t getBaseline() { return baseline_us; }
  static inline uint16_t sampleRaw()   { return measure_us(); }
  static inline bool     state()       { return touched; }
}

// ================== API simple (utilisée par main.cpp) ==================
static inline void Touch_begin()             { TouchCap::begin(); }
static inline bool Touch_isTouched()         { return TouchCap::isTouched(); }
// Debug facultatif :
static inline uint16_t Touch_getBaseline_us(){ return TouchCap::getBaseline(); }
static inline uint16_t Touch_getRaw_us()     { return TouchCap::sampleRaw(); }
static inline bool Touch_state()             { return TouchCap::state(); }