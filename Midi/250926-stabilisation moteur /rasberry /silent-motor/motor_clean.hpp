#pragma once
#include <Arduino.h>

// Clean DRV8871 driver for RP2040 (Pico)
// - One PWM pin (IN1) and one DIR pin (IN2).
// - "Coast" at rest (both low).
// - Supports high PWM freq to avoid audible noise.

struct DRV8871 {
  uint8_t pin_pwm;   // IN1
  uint8_t pin_dir;   // IN2
  uint32_t pwm_hz;   // default 40000 Hz

  // Limits & behavior
  float duty_min = 0.08f;   // friction compensation
  float duty_max = 0.80f;   // clamp
  bool  slow_decay = true;  // if true: PWM on IN1 and level on IN2

  void begin(uint8_t in1_pwm, uint8_t in2_dir, uint32_t pwm_freq_hz=40000) {
    pin_pwm = in1_pwm; pin_dir = in2_dir; pwm_hz = pwm_freq_hz;
    pinMode(pin_pwm, OUTPUT);
    pinMode(pin_dir, OUTPUT);
    digitalWrite(pin_pwm, LOW);
    digitalWrite(pin_dir, LOW);
    setPwmFreq(pwm_hz);
  }

  void setPwmFreq(uint32_t hz) {
    // Earle Philhower Arduino-Pico supports analogWriteFreq()
    #if defined(ARDUINO_ARCH_RP2040)
      analogWriteFreq(hz);
    #endif
  }

  void coast() {
    analogWrite(pin_pwm, 0);
    digitalWrite(pin_dir, LOW);
  }

  // u in [-1..+1]
  void drive(float u) {
    if (fabsf(u) < 1e-6f) { coast(); return; }
    float s = constrain(u, -1.0f, 1.0f);

    // Apply clamps
    float mag = fabsf(s);
    if (mag > 0.0f && mag < duty_min) mag = duty_min;
    if (mag > duty_max) mag = duty_max;

    uint16_t duty = (uint16_t)(mag * 255.0f);

    if (s >= 0) {
      // Direction A
      digitalWrite(pin_dir, LOW);
      analogWrite(pin_pwm, duty);
    } else {
      // Direction B
      digitalWrite(pin_dir, HIGH);
      analogWrite(pin_pwm, duty);
    }
  }
};
