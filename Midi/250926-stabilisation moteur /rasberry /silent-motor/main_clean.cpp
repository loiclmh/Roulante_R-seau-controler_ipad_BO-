#include "config.h"
#ifndef RUN_NOISE_SWEEP
#define RUN_NOISE_SWEEP 0
#endif
#if !RUN_NOISE_SWEEP
// main_clean.cpp — minimal closed-loop step test (no display)
// - Reads fader ADC (0..1023), follows a step profile
// - PID tuned for silence: coast inside deadband, high PWM freq
// - Prints CSV over Serial: time_ms,setpoint,position,u_cmd

#include <Arduino.h>
#include "pins.h"
#include "motor_clean.hpp"
#include "pid_clean.hpp"
#include "display_oled.hpp"

DRV8871   drv;
SimplePID pid;
OledUi oled;

// ADC helpers
uint16_t read_fader() {
  return analogRead(PIN_FADER_ADC); // 0..1023 on RP2040 Arduino core
}

// Map PID output [-1000..1000] to drive [-1..+1]
float map_u(int16_t u) {
  float s = (float)u / 1000.0f;
  if (fabsf(s) < 0.01f) return 0.0f;
  return s;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // OLED init
  oled.begin();

  // ADC
  analogReadResolution(10);          // 0..1023
  analogReadPin(PIN_FADER_ADC);

  // Motor
  drv.begin(PIN_DRV8871_PWM, PIN_DRV8871_DIR, 40000);
  drv.duty_min = 0.08f;
  drv.duty_max = 0.80f;

  // PID
  pid.setTunings(1.1f, 0.0f, 0.04f, 60.0f, 1000.0f);

  Serial.println("#STEP_TEST,ver=1");
  Serial.println("#columns: t_ms,setpoint,position,u_cmd");
}

uint32_t t0=0;
int16_t setpoint=0;
uint8_t phase=0;

void loop() {
  static uint32_t last = 0;
  uint32_t now = millis();
  if (now - last < 1) return;  // 1 kHz loop
  last = now;

  // Generate a step profile every 2s: 200 -> 800 -> 200...
  if (t0==0) t0 = now;
  uint32_t t = now - t0;
  if (t % 2000 < 20) {
    phase ^= 1;
    setpoint = phase ? 800 : 200;
  }

  uint16_t pos = read_fader();
  int16_t u = pid.update(setpoint, pos);

  // Deadband near zero error => coast to keep silent
  if (abs(setpoint - (int)pos) < 10) {
    drv.coast();
  } else {
    drv.drive(map_u(u));
  }


  // OLED ~30 Hz
  static uint32_t last_oled = 0;
  if (now - last_oled >= 33) {
    last_oled = now;
    oled.draw(pos, setpoint, u, drv.pwm_hz);
  }

    // Log CSV (downsample to ~100 Hz for serial)
  static uint8_t decim=0;
  if (++decim >= 10) {
    decim = 0;
    Serial.print(now); Serial.print(",");
    Serial.print(setpoint); Serial.print(",");
    Serial.print(pos); Serial.print(",");
    Serial.println(u);
  }
}

#endif
