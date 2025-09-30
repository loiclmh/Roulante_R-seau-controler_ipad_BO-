#pragma once
#include <Arduino.h>

// Simple PID with derivative on measurement and a 1st-order LP filter
// tuned for 1 kHz loop by default.
struct SimplePID {
  float kp=1.0f, ki=0.0f, kd=0.0f;
  float Ts=0.001f;         // 1 kHz
  float fc=60.0f;          // derivative LP cutoff (Hz)
  // state
  float i_acc=0.0f;
  float d_prev=0.0f;
  float a1=0.0f;

  void setTunings(float Kp,float Ki,float Kd,float fc_hz,float loop_hz) {
    kp=Kp; ki=Ki; kd=Kd; Ts=1.0f/loop_hz; fc=fc_hz;
    const float RC = 1.0f/(2.0f*PI*max(1.0f,fc));
    a1 = RC/(RC + Ts);
  }

  void reset() { i_acc=0.0f; d_prev=0.0f; }

  // setpoint & meas in 0..1023 ; returns command in [-1000..1000]
  int16_t update(int16_t sp, int16_t meas) {
    const float e = (float)(sp - meas);        // error
    const float up = kp * e;

    // integral with simple clamp
    i_acc += ki * e * Ts;
    i_acc = constrain(i_acc, -400.0f, 400.0f);

    // derivative on measurement (with sign for -d(meas)/dt)
    float d_meas = -(float)meas;
    d_prev = a1*d_prev + (1.0f-a1)*d_meas;
    float ud = kd * d_prev * 0.01f; // scale to keep small

    float u = up + i_acc + ud;
    if (u > 1000.0f) u = 1000.0f;
    if (u < -1000.0f) u = -1000.0f;
    return (int16_t)u;
  }
};
