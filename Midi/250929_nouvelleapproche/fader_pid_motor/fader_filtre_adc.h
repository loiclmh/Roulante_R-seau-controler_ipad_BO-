#pragma once
#include <cstdint>
#include <Arduino.h>

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

//====================== amélioration possible  =====================
/*
    le code est uttilsiable que par des valeur en 12 bits (0-4095)
    pour l'utiliser en 10 bits (0-1023) il faut modifier la ligne
    les butées USABLE_MIN et USABLE_MAX ne marche pas 
    il faut crée des variable en % pour qu'il soit fonctionel sur tout les format
*/


// ===================== RÉGLAGES (tout en haut) =====================
constexpr uint8_t MAX_FADERS = 4;   // limite dure (ne pas dépasser)
constexpr uint8_t NUM_FADERS = 1;    // ← règle ici (1..11)
constexpr int DEADBAND_ADC = 8;  // ajuste 2-8 selon tolérance

// Liste des broches ADC pour jusqu’à 11 faders (RP2040 : GP26=ADC0, GP27=ADC1, GP28=ADC2, GP29=ADC3)
constexpr uint8_t FADER_PINS[MAX_FADERS] = { A0, A1, A2, A3 };

// ADC (RP2040 : 12 bits → 0..4095)
constexpr int MY_ADC_BITS = 12;
constexpr int ADC_MAX  = (1 << MY_ADC_BITS) - 1;

// Filtre (type Control Surface). Plus grand = plus lisse (plus lent)
// NOTE: avec Control Surface, FILTER_SHIFT est un paramètre de compilation (template).
// Modifie la valeur ici puis recompile.
constexpr uint8_t FILTER_SHIFT = 3;  // 3 très réactif, 4-6 plus doux

// Zones mortes + snap 
constexpr int  USABLE_MIN   = 10;    // marge basse (butée mécanique)
constexpr int  USABLE_MAX   = 4060;  // marge haute (butée mécanique)
constexpr int  snap_low    = 8 ; // valeur en dessous de laquel le fader se met a 0
constexpr int  snap_high   = 4080 ; // valeur au dessus de laquel le fader se met a 4095

// Cadence
constexpr uint8_t LOOP_DELAY_MS = 2;

extern uint16_t gFaderADC[MAX_FADERS]; // valeurs filtrées brutes 0..4095
extern uint8_t fader_idx;    // fader/moteur à tester/envoyer

void setupADC();
void loopfader(uint8_t i);