#pragma once
#include <stdint.h>

struct PID {
  static constexpr float Ts = 0.001f; // 1 ms

  static constexpr float KP_DEFAULT = 2.80f;
  static constexpr float KI_DEFAULT = 0.67f;
  static constexpr float KD_DEFAULT = 0.0027f;
  static constexpr float FC_DEFAULT = 60.0f;

  static constexpr float KI_SCALE = 100.0f;
  static constexpr float KD_SCALE = 0.01f;

  float kp = KP_DEFAULT;
  float ki = KI_DEFAULT;
  float kd = KD_DEFAULT;
  float a1 = 0.0f;
  float i_acc = 0.0f;
  float d_prev = 0.0f;

  PID() {
    setTunings(kp, ki, kd, FC_DEFAULT);
    reset();
  }

  void setTunings(float Kp, float Ki, float Kd, float fc_hz);
  void reset();
  int16_t update(int16_t setpoint, int16_t meas);
  void applyDefaults();
};
