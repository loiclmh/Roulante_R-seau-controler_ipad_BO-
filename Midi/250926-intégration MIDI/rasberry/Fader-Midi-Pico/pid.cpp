#include "pid.h"
#include <math.h>

void PID::setTunings(float Kp, float Ki, float Kd, float fc_hz) {
  kp = Kp;
  ki = Ki;
  kd = Kd;
  // filtre dérivé : y[n] = a1*y[n-1] + (1-a1)*x[n]
  if (fc_hz <= 0) { a1 = 0.0f; }
  else {
    float RC = 1.0f / (2.0f * M_PI * fc_hz);
    a1 = RC / (RC + Ts);
    if (a1 < 0) a1 = 0; if (a1 > 0.9999f) a1 = 0.9999f;
  }
}

void PID::reset() {
  i_acc = 0.0f;
  d_prev = 0.0f;
}

int16_t PID::update(int16_t setpoint, int16_t meas) {
  // Erreur
  float e = (float)setpoint - (float)meas;

  // Proportionnel
  float up = kp * e;

  // Intégral (anti-windup simple)
  i_acc += ki * e * Ts * 100.0f; // *100 pour ramener les ordres de grandeur
  if (i_acc > 1000.0f) i_acc = 1000.0f;
  if (i_acc < -1000.0f) i_acc = -1000.0f;

  // Dérivé (sur la mesure -> -kd * d(y)/dt ; on l’approx avec filtre)
  float d_meas = - (float)(meas);        // signe pour d(meas)
  d_prev = a1 * d_prev + (1.0f - a1) * d_meas;
  float ud = kd * (d_prev) * 0.01f;      // *0.01 pour calmer la contribution

  // Somme
  float u = up + i_acc + ud;

  // Saturation de sortie
  if (u > 1000.0f) u = 1000.0f;
  if (u < -1000.0f) u = -1000.0f;

  return (int16_t)u;
}
