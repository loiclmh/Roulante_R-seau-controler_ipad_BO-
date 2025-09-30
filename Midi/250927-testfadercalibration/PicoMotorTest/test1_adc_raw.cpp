#include <Arduino.h>
#include "pins.h"
#include "display.h"


// --- Paramètres de filtrage et stabilité ---
const int NS = 150;   // nombre d’échantillons pour la moyenne (suréchantillonnage)
const int HYST = 25;  // hystérésis pour éviter le jitter visuel (plage ~6 counts)

// --- Bornes utilisables pour mapping sûr ---
const int USABLE_MIN = 40;   // zone morte basse
const int USABLE_MAX = 4020; // zone morte haute


// Test 1 : lecture fader 12 bits fixes + suréchantillonnage ×12 + médiane + EMA + hystérésis
static inline uint32_t us_now() { return micros(); }

static int median3(int a, int b, int c) {
  if (a > b) { int t=a; a=b; b=t; }
  if (b > c) { int t=b; b=c; c=t; }
  if (a > b) { int t=a; a=b; b=t; }
  return b;
}

void run_test1() {
  static bool inited = false;
  static uint32_t next_disp = 0;
  static uint32_t next_log  = 0;

  if (!inited) {
    display_show_raw(0);
    next_disp = us_now();
    next_log  = us_now();
    inited = true;
  }

  // --- Acquisition robuste ---
  int buf[NS];
  long acc = 0;
  for (int i = 0; i < NS; ++i) {
    int v = analogRead(PIN_FADER_ADC); // 0..4095 si analogReadResolution(12) est actif
    buf[i] = v;
    acc += v;
  }

  int m3   = median3(buf[NS-3], buf[NS-2], buf[NS-1]); // coupe les pics isolés
  int avg  = (int)(acc / NS);                           // réduit le bruit blanc
  int raw12 = (m3 + avg) / 2;                           // mix anti-pics

  // --- EMA très léger ---
  static int filt12 = -1;
  if (filt12 < 0) filt12 = raw12;
  // alpha = 1/8 → latence ~1–2 ms @1 kHz
  filt12 = ((filt12 * 7) + raw12) / 8;

  // --- Hystérésis ---
  static int out12 = -1;
  if (out12 < 0 || abs(filt12 - out12) > HYST) out12 = filt12;

  uint32_t now = us_now();

  // OLED ~25 Hz
  if ((int32_t)(now - next_disp) >= 0) {
    next_disp = now + 40000;
    display_show_raw(out12, USABLE_MIN, USABLE_MAX);
  }

  // Serial ~10 Hz
  if ((int32_t)(now - next_log) >= 0) {
    next_log = now + 100000;
    int out8 = constrain(map(out12, USABLE_MIN, USABLE_MAX, 0, 255), 0, 255);
    Serial.print("AVG=");   Serial.print(avg);
    Serial.print("\tM3=");  Serial.print(m3);
    Serial.print("\tFILT=");Serial.print(filt12);
    Serial.print("\tOUT="); Serial.print(out12);
    Serial.print("\tOUT8="); Serial.println(out8);
  }
}
