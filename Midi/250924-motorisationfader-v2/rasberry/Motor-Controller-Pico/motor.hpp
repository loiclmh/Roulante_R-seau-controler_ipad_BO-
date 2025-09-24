#pragma once
#include <Arduino.h>
#include "pins.h"

namespace Motor {
  inline uint16_t magClip(int32_t x, uint16_t maxv) {
    if (x < 0) x = -x;
    if (x > maxv) x = maxv;
    return (uint16_t)x;
  }

  inline void begin() {
    pinMode(PIN_DRV8871_PWM, OUTPUT); // IN1
    pinMode(PIN_DRV8871_DIR, OUTPUT); // IN2
    analogWrite(PIN_DRV8871_PWM, 0);
    analogWrite(PIN_DRV8871_DIR, 0);
    // Option: analogWriteFreq(31000);
  }

  inline void drive(int16_t u) {
    // Deadband
    if (u > -12 && u < 12) {
      analogWrite(PIN_DRV8871_PWM, 0);
      analogWrite(PIN_DRV8871_DIR, 0);
      return;
    }
    uint16_t mag = magClip(u, 1000);
    uint8_t  pwm = map(mag, 0, 1000, 0, 255);

    if (u > 0) {
      analogWrite(PIN_DRV8871_PWM, pwm);  // IN1
      analogWrite(PIN_DRV8871_DIR, 0);    // IN2
    } else {
      analogWrite(PIN_DRV8871_PWM, 0);          // IN1
      analogWrite(PIN_DRV8871_DIR, (int)pwm);   // IN2
    }
  }

  inline void stop() {
    analogWrite(PIN_DRV8871_PWM, 0);
    analogWrite(PIN_DRV8871_DIR, 0);
  }
}
