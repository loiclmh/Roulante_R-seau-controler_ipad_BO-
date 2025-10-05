#include "debug.h"
#include <Arduino.h>

// ================================= variable pour débug programe =========================

// variable pour activer les print de debug dans les fichiers .cpp
bool on_debug = true;  // active ou non les print dans pid.cpp
bool on_debug_python = true; // active ou non les print dans motor.cpp
bool on_debug_monitorarduino = false; // active ou non les print dans bash_test.cpp

// ======================== variable pour le mode test =========================
// 0 = OFF (rien), 1 = TEST LOCAL (séquence 5-50-25-95-75-0), 2 = PYTHON (réglages via série)
uint8_t bash_test_mode = 2; 

//====================== DEBUG FADER ADC =====================
int debugOLED_fader = 1; // 0=off, 1=affiche valeurs ADC dans le moniteur série
int debug_fadermoniteur = 0; // 0=off, 1=affiche ready sur le moniteur


void debugsetup() {
  if (on_debug) return;
  else {
  on_debug_python = false;
  on_debug_monitorarduino = false;
  debugOLED_fader = 0;
  debug_fadermoniteur = 0;
  }
}