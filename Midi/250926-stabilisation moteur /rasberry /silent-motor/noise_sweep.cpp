#include "config.h"
#ifndef RUN_NOISE_SWEEP
#define RUN_NOISE_SWEEP 0
#endif
#if RUN_NOISE_SWEEP
// tests/noise_sweep.cpp
// Purpose: find "noise onset" vs PWM frequency and duty_min.
// - Press the button on GP2 when you HEAR noise -> it logs the current step.
// - Results are printed over USB Serial as CSV for later analysis.
// Compile as Arduino sketch (RP2040 core).

#include <Arduino.h>
#include "pins.h"
#include "motor_clean.hpp"

DRV8871 drv;

static const uint32_t freqs[] = { 20000, 25000, 32000, 40000, 50000 };
static const float duty_mins[] = { 0.04f, 0.06f, 0.08f, 0.10f, 0.12f };

uint8_t buttonState() { pinMode(PIN_BUTTON_NOISE, INPUT_PULLUP); return digitalRead(PIN_BUTTON_NOISE); }

void setup() {
  Serial.begin(115200);
  delay(400);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  drv.begin(PIN_DRV8871_PWM, PIN_DRV8871_DIR, freqs[0]);
  drv.duty_max = 0.80f;

  Serial.println("#NOISE_TEST,ver=1");
  Serial.println("#columns: freq_hz,duty_min,step,u_signed,button");
}

void do_step(float u, uint32_t dwell_ms) {
  drv.drive(u);
  uint8_t b = (buttonState()==LOW)?1:0;
  Serial.print(drv.pwm_hz); Serial.print(",");
  Serial.print(drv.duty_min, 3); Serial.print(",");
  static uint32_t step=0; Serial.print(step++); Serial.print(",");
  Serial.print(u, 3); Serial.print(",");
  Serial.println((int)b);
  digitalWrite(LED_PIN, b?HIGH:LOW);
  delay(dwell_ms);
}

void loop() {
  for (uint8_t fi=0; fi<sizeof(freqs)/sizeof(freqs[0]); ++fi) {
    drv.setPwmFreq(freqs[fi]);
    drv.pwm_hz = freqs[fi];
    for (uint8_t di=0; di<sizeof(duty_mins)/sizeof(duty_mins[0]); ++di) {
      drv.duty_min = duty_mins[di];

      // Sweep small steps around zero to probe stick/slip noise
      for (float u=0.00f; u<=0.30f; u+=0.02f)  do_step(+u, 180);
      for (float u=0.30f; u>=-0.30f; u-=0.02f) do_step(+u, 120);
      for (float u=-0.30f; u<=0.00f; u+=0.02f) do_step(+u, 180);

      // Rest
      drv.coast();
      delay(400);
    }
  }

  // Hold done
  drv.coast();
  while (1) { digitalWrite(LED_PIN, HIGH); delay(200); digitalWrite(LED_PIN, LOW); delay(800); }
}

#endif
