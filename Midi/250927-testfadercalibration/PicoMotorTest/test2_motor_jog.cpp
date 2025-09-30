#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "motor.h"

// Réglages test 2
static const uint32_t MOTOR_PWM_FREQ_HZ = 25000; // change ici pour écouter le bruit (ex. 31000, 48000)
static const uint8_t  DUTY_ABS          = 140;   // 0..255 (~55%) pour le test
static const uint16_t DWELL_MS          = 1200;  // temps d'écoute à chaque palier
static const bool     ZERO_BRAKE        = false; // false=coast, true=brake

static inline int read_fader_raw12() {
  long acc=0;
  for (int i=0;i<8;i++) acc += analogRead(PIN_FADER_ADC);
  return (int)(acc/8);
}

void run_test2() {
  static bool inited=false;
  static int phase = 0; // 0=stop,1=fwd,2=stop,3=rev
  static uint32_t tNext=0;

  if (!inited) {
    motor_begin(/*IN1*/16, /*IN2*/17, MOTOR_PWM_FREQ_HZ);
    if (ZERO_BRAKE) motor_brake(); else motor_coast();
    inited = true;
    tNext = millis();
  }

  int raw = read_fader_raw12();
  int out8 = map(raw, 0, 4095, 0, 255);

  const char* dir = "STOP";
  switch (phase) {
    case 1: dir = "FWD"; break;
    case 3: dir = "REV"; break;
    default: dir = ZERO_BRAKE ? "STOP(BRAKE)" : "STOP(COAST)"; break;
  }
  display_show_fader_and_dir(raw, 0, 4095, dir, out8);

  if ((int32_t)(millis() - tNext) < 0) return;
  tNext = millis() + DWELL_MS;

  if (phase == 0) {
    motor_drive(+DUTY_ABS); phase = 1;
  } else if (phase == 1) {
    motor_drive(0);         phase = 2;
  } else if (phase == 2) {
    motor_drive(-DUTY_ABS); phase = 3;
  } else {
    motor_drive(0);         phase = 0;
  }
}
