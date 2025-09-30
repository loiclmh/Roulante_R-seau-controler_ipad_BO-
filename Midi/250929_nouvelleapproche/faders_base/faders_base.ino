/*
  Faders multi-canaux (RP2040) — Control Surface + zones mortes
  - Nombre de faders réglable via NUM_FADERS (1..11)
  - Filtre Control Surface (FilteredAnalog) si dispo, sinon filtre exponentiel simple
  - Zones mortes :
      1) Marges de butées : USABLE_MIN / USABLE_MAX (ignore un peu les extrémités mécaniques)
      2) Petite zone morte de stabilité : DEADBAND_CC (ignorer les minis variations en 7 bits)
  - Sortie 0..127 (mapping 12 bits → 7 bits)

  Carte: RP2040 (Raspberry Pi Pico / Seeeduino XIAO RP2040)

                       +3V3 ─────┬───────────────┐
                                 │               │
                                === 100 µF       │  (gros condo pour stabiliser l’alim)
                                --- 16 V         │
                                 │               │
          GND ───────────────────┴───────────────┘

                +3V3 ────────────────┐
                                     │
                                 [ POTAR ]
                                 /\/\/\/\← curseur (wiper)
                                     │
                                     ├─────┬───────────→ vers A0 (entrée ADC)
                                     │     │
                                     │    === 100 nF
                                      │     │
          GND ──────────┬────────────┴─────┘
                      220 Ω
                        │
          GND ───────────┘

*/

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ====== Control Surface (obligatoire) ======
#include <Arduino_Helpers.h>
#include <AH/Hardware/FilteredAnalog.hpp>

//====================== DEBUG =====================
constexpr int debug_fader = 0; // 0=off, 1=affiche valeurs ADC dans le moniteur série
constexpr int debug_fadermoniteur = 0; // 0=off, 1=affiche ready sur le moniteur 


//====================== écran oled  =====================
void setupOLED() {
  if (debug_fader != 1) return; // ne rien faire si debug off
  Wire.setSDA(4);                 // SDA
  Wire.setSCL(5);                 // SCL
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Échec init SSD1306 @0x3C — vérifie SDA=4 SCL=5 et l'alim.");
    return;
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
}


// ===================== RÉGLAGES (tout en haut) =====================
constexpr uint8_t MAX_FADERS = 4;   // limite dure (ne pas dépasser)
constexpr uint8_t NUM_FADERS = 1;    // ← règle ici (1..11)
constexpr int DEADBAND_ADC = 4;  // ajuste 2-8 selon tolérance

// Liste des broches ADC pour jusqu’à 11 faders (RP2040 : GP26=ADC0, GP27=ADC1, GP28=ADC2, GP29=ADC3)
constexpr uint8_t FADER_PINS[MAX_FADERS] = { A0, A1, A2, A3 };

// ADC (RP2040 : 12 bits → 0..4095)
constexpr int MY_ADC_BITS = 12;
constexpr int ADC_MAX  = (1 << MY_ADC_BITS) - 1;

// Filtre (type Control Surface). Plus grand = plus lisse (plus lent)
// NOTE: avec Control Surface, FILTER_SHIFT est un paramètre de compilation (template).
// Modifie la valeur ici puis recompile.
constexpr uint8_t FILTER_SHIFT = 2;  // 3 très réactif, 4-6 plus doux

// Zones mortes
constexpr int  USABLE_MIN   = 0;    // marge basse (butée mécanique)
constexpr int  USABLE_MAX   = 4095;  // marge haute (butée mécanique)

// Cadence
constexpr uint8_t LOOP_DELAY_MS = 2;

// ===================== ÉTAT =====================
// Un filtre Control Surface par fader
static AH::FilteredAnalog<MY_ADC_BITS, FILTER_SHIFT, uint32_t>* gFaders[MAX_FADERS] = {nullptr};
static int gFaderADC[MAX_FADERS] = {0}; // valeurs filtrées brutes 0..4095

// ===================== UTILS =====================
static inline int clamp(int v, int lo, int hi) {
  return (v < lo) ? lo : (v > hi) ? hi : v;
}

// ===================== SETUP/LOOP =====================
void setup() {
  static_assert(NUM_FADERS >= 1 && NUM_FADERS <= MAX_FADERS, "NUM_FADERS doit être entre 1 et MAX_FADERS");

  analogReadResolution(MY_ADC_BITS);
  Serial.begin(115200);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 1500) {}

  // Init faders avec filtre Control Surface
  for (uint8_t i = 0; i < NUM_FADERS; ++i) {
    pinMode(FADER_PINS[i], INPUT);
  gFaders[i] = new AH::FilteredAnalog<MY_ADC_BITS, FILTER_SHIFT, uint32_t>(FADER_PINS[i]);
  gFaders[i]->update();
  int raw = (int)gFaders[i]->getValue();
    raw = clamp(raw, USABLE_MIN, USABLE_MAX);
    gFaderADC[i] = raw;
  }
  setupOLED();
  if  (debug_fadermoniteur == 1) { // ne s'active que si debug_fader=1
    Serial.println("READY");
  }
}

void loop() {
  for (uint8_t i = 0; i < NUM_FADERS; ++i) {
    // 1. Met à jour le filtre Control Surface puis récupère la valeur
    gFaders[i]->update();
    int filt_raw = (int)gFaders[i]->getValue();
    int raw_direct = analogRead(FADER_PINS[i]);
    if (debug_fader == 1) { // ne s'active que si debug_fader=1
      Serial.print("raw="); Serial.print(filt_raw); Serial.print(" ");
      Serial.print(" rawCS="); Serial.print(filt_raw);
      Serial.print(" rawADC="); Serial.print(raw_direct);
      }
    // 2. Application de la zone morte mécanique
    filt_raw = clamp(filt_raw, USABLE_MIN, USABLE_MAX);
    // --- Deadband logiciel pour stabiliser ---
    if (abs(filt_raw - gFaderADC[i]) < DEADBAND_ADC) {
      filt_raw = gFaderADC[i]; // conserve l'ancienne valeur si variation minime
    }
    // 3. Stockage dans gFaderADC[i]
    gFaderADC[i] = filt_raw;

    if (debug_fader == 1) { // ne s'active que si debug_fader=1
      Serial.print("Fader"); Serial.print(i);
      Serial.print("_ADC=");
      Serial.println(gFaderADC[i]);
    }
  }
  
  if (debug_fader == 1) { // ne s'active que si debug_fader=1
  // Affichage OLED du premier fader
  drawOLED(gFaderADC[0]);
  delay(LOOP_DELAY_MS);
  }
}

void drawOLED(int value) {
  // Affiche la valeur du fader 0 passée en paramètre
  oled.clearDisplay();

  oled.setCursor(0, 0);
  oled.print("ADC: ");
  oled.println(value);

  int barLength = map(value, 0, 4095, 0, SCREEN_WIDTH);
  oled.fillRect(0, 20, barLength, 10, SSD1306_WHITE);

  oled.display();
}