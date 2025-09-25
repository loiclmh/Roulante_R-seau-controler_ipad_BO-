#pragma once
#include <Arduino.h>
#include "pins.h"

namespace Motor {

  // --- paramètres bruit/adhérence ---
  static constexpr int16_t  DEAD_BAND = 15;   // fenêtre ± autour de 0
  static constexpr uint8_t  PWM_FLOOR = 30;   // plancher PWM pour décoller le moteur
  static constexpr uint16_t MAX_MAG   = 1000; // amplitude logique maximale (échelle de u)

  inline uint16_t magClip(int32_t x, uint16_t maxv) {
    if (x < 0) x = -x;
    if ((uint32_t)x > maxv) x = maxv;
    return (uint16_t)x;
  }

  inline void begin() {
    pinMode(PIN_DRV8871_PWM, OUTPUT); // IN1 = PWM
    pinMode(PIN_DRV8871_DIR, OUTPUT); // IN2 = DIR (digital)
    digitalWrite(PIN_DRV8871_PWM, LOW);
    digitalWrite(PIN_DRV8871_DIR, LOW);

    // PWM ultrasonique pour RP2040
    #if defined(ARDUINO_ARCH_RP2040)
      analogWriteFreq(31000); // ≈31 kHz
    #endif
  }

  inline void drive(int16_t u) {
    // 1) Deadband → arrêt (coast)
    if (u > -DEAD_BAND && u < DEAD_BAND) {
      analogWrite(PIN_DRV8871_PWM, 0); // IN1 = 0
      analogWrite(PIN_DRV8871_DIR, 0); // IN2 = 0
      return;
    }

    // 2) Magnitude → PWM avec plancher pour décoller
    uint16_t mag = magClip(u, MAX_MAG);
    uint8_t  pwm = map(mag, 0, MAX_MAG, 0, 255);
    if (pwm && pwm < PWM_FLOOR) pwm = PWM_FLOOR;

    // 3) Sélection de sens pour DRV8871
    if (u > 0) {
      // Avant: IN1 = PWM, IN2 = LOW (slow decay)
      analogWrite(PIN_DRV8871_PWM, pwm);
      analogWrite(PIN_DRV8871_DIR, 0);
    } else {
      // Arrière: IN1 = LOW, IN2 = PWM (slow decay)
      analogWrite(PIN_DRV8871_PWM, 0);
      analogWrite(PIN_DRV8871_DIR, pwm);
    }
  }

  inline void stop() {
    analogWrite(PIN_DRV8871_PWM, 0);
    digitalWrite(PIN_DRV8871_DIR, LOW);
  }
}