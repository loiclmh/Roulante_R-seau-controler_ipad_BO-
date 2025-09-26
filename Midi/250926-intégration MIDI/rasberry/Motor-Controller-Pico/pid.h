#pragma once
#include <Arduino.h>

class PID {
public:
  // fs = 1000 Hz (dt = 1 ms) dans notre loop
  void setTunings(float Kp, float Ki, float Kd, float fc_hz);
  void reset();

  // setpoint & measurement en unités 0..1023 ; retour dans ~[-1000; +1000]
  int16_t update(int16_t setpoint, int16_t meas);

private:
  float kp = 0, ki = 0, kd = 0;
  float Ts = 0.001f;      // 1 kHz
  // Dérivée filtrée (passe-bas 1er ordre, fc)
  float a1 = 0.0f;        // coeff filtre dérivée
  float d_prev = 0.0f;

  float i_acc = 0.0f;     // intégrateur
};
