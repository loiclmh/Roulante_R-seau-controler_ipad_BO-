// Fader_info.cpp
// Copie du pipeline de Test 1, sans aucun affichage OLED.
// Utilise exclusivement les constantes *_CAL depuis calibration.h
// Permet d'observer les valeurs sur le port série (RAW12, FILT12, OUT, OUT8).

#include <Arduino.h>
#include "pins.h"
#include "calibration.h"

static inline uint32_t us_now() { return micros(); }

// Appeler cette fonction régulièrement (depuis loop) si tu veux l'utiliser
void fader_info_tick() {
  static bool inited = false;
  static uint32_t next_log = 0;

  if (!inited) {
    next_log = us_now();
    inited = true;
  }

  // Lecture avec suréchantillonnage
  long acc = 0;
  for (int i = 0; i < NS; ++i) {
    acc += analogRead(PIN_FADER_ADC);
  }
  int raw12 = (int)(acc / NS);

  // EMA léger
  static int filt12 = -1;
  if (filt12 < 0) filt12 = raw12;
  filt12 = ((filt12 * 7) + raw12) / 8;  // alpha = 1/8

  // Hystérésis
  static int out12 = -1;
  if (out12 < 0 || abs(filt12 - out12) > HYST) out12 = filt12;

  // Logging toutes les ~100 ms
  uint32_t now = us_now();
  if ((int32_t)(now - next_log) >= 0) {
    next_log = now + 100000;

    int out8 = constrain(map(out12, FADER_USABLE_MIN_CAL, FADER_USABLE_MAX_CAL, 0, 255), 0, 255);
    Serial.print("[FADER_INFO] ");
    Serial.print("RAW12=");  Serial.print(raw12);
    Serial.print("\tFILT12="); Serial.print(filt12);
    Serial.print("\tOUT=");    Serial.print(out12);
    Serial.print("\tOUT8=");   Serial.println(out8);
  }
}

// Variante: lecture unique (utile si tu veux l'appeler à la demande)
void fader_info_read_once() {
  long acc = 0;
  for (int i = 0; i < NS; ++i) acc += analogRead(PIN_FADER_ADC);
  int raw12 = (int)(acc / NS);
  static int filt12 = -1;
  if (filt12 < 0) filt12 = raw12;
  filt12 = ((filt12 * 7) + raw12) / 8;
  static int out12 = -1;
  if (out12 < 0 || abs(filt12 - out12) > HYST) out12 = filt12;
  int out8 = constrain(map(out12, FADER_USABLE_MIN_CAL, FADER_USABLE_MAX_CAL, 0, 255), 0, 255);

  Serial.print("[FADER_INFO] ");
  Serial.print("RAW12=");  Serial.print(raw12);
  Serial.print("\tFILT12="); Serial.print(filt12);
  Serial.print("\tOUT=");    Serial.print(out12);
  Serial.print("\tOUT8=");   Serial.println(out8);
}
