#include "pid.h"

static constexpr float TWO_PI_F = 6.28318530718f;

void PID::setTunings(float Kp, float Ki, float Kd, float fc_hz) {
  kp = Kp;
  ki = Ki;
  kd = Kd;

  if (fc_hz <= 0.0f) {
    a1 = 0.0f;
  } else {
    const float RC = 1.0f / (TWO_PI_F * fc_hz);
    a1 = RC / (RC + Ts);
    if (a1 < 0.0f) a1 = 0.0f;
    else if (a1 > 0.9999f) a1 = 0.9999f;
  }
}

void PID::reset() {
  i_acc = 0.0f;
  d_prev = 0.0f;
}

int16_t PID::update(int16_t setpoint, int16_t meas) {
  const float e = (float)setpoint - (float)meas;

  const float up = kp * e;

  i_acc += ki * e * Ts * KI_SCALE;
  if (i_acc > 1000.0f) i_acc = 1000.0f;
  if (i_acc < -1000.0f) i_acc = -1000.0f;

  float d_meas = -(float)meas;
  d_prev = a1 * d_prev + (1.0f - a1) * d_meas;
  const float ud = kd * d_prev * KD_SCALE;

  float u = up + i_acc + ud;
  if (u > 1000.0f) u = 1000.0f;
  if (u < -1000.0f) u = -1000.0f;

  return (int16_t)u;
}

void PID::applyDefaults() {
  setTunings(KP_DEFAULT, KI_DEFAULT, KD_DEFAULT, FC_DEFAULT);
  reset();
}
