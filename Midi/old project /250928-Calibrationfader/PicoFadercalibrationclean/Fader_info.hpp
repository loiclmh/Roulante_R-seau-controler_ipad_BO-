#include <Arduino.h>
#include "pins.h"
#include "display.h"
#include "comms.h"

// --- Constantes au début pour réglage facile ---
const int NS = 150;         // suréchantillonnage (échantillons)
const int HYST = 25;        // hystérésis (counts ADC)
const int USABLE_MIN = 40;  // borne basse utilisée
const int USABLE_MAX = 4025;// borne haute utilisée

// Filtre EMA
static int ema = -1;

// Convertit RAW 12 bits -> [0..255] en tenant compte des bornes
static int map_to_8bit(int raw) {
  int clamped = raw;
  if (clamped < USABLE_MIN) clamped = USABLE_MIN;
  if (clamped > USABLE_MAX) clamped = USABLE_MAX;
  long span = (long)USABLE_MAX - (long)USABLE_MIN;
  if (span < 1) span = 1;
  long v = ((long)(clamped - USABLE_MIN) * 255L) / span;
  if (v < 0) v = 0;
  if (v > 255) v = 255;
  return (int)v;
}

void run_test1() {
  // Lecture suréchantillonnée
  long acc = 0;
  for (int i = 0; i < NS; ++i) {
    acc += analogRead(PIN_FADER);
  }
  int raw = (int)(acc / NS);

  // EMA first-order
  if (ema < 0) ema = raw;
  ema = (ema * 7 + raw * 1) / 8;  // alpha = 1/8

  // Hystérésis simple
  static int shown = 0;
  if (abs(ema - shown) > HYST) shown = ema;

  // Mapping vers 8 bits
  int out8 = map_to_8bit(shown);

  // Affichage OLED
  display_fader(shown, USABLE_MIN, USABLE_MAX, "T1", out8);

  // Sorties série (pour script Python)
  comms_kv("RAW", shown);
  comms_kv("OUT8", out8);

  // Protocole RX simple
  if (comms_available_line()) {
    String line = comms_take_line();
    line.trim();
    if (line.equalsIgnoreCase("PING")) {
      Serial.println("PONG");
    } else if (line.startsWith("SET USABLE_MIN=")) {
      int v = line.substring(14).toInt();
      // Note: constantes sont const ici; pour runtime, les rendre variables.
      Serial.println("ERR:USABLE_MIN is const at compile time");
    } else if (line.startsWith("SET USABLE_MAX=")) {
      int v = line.substring(14).toInt();
      Serial.println("ERR:USABLE_MAX is const at compile time");
    } else if (line.equalsIgnoreCase("GET")) {
      Serial.print("RAW="); Serial.print(shown);
      Serial.print(" OUT8="); Serial.println(out8);
    } else {
      Serial.print("ECHO: "); Serial.println(line);
    }
  }

  delay(5);
}
