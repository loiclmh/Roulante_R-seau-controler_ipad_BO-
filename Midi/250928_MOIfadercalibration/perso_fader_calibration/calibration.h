#pragma once

// Valeur de TEST choisie ailleurs (ex: main.ino)
// #define TEST 1  // 0=normal, 1=test (si tu veux forcer ici)

#ifndef NUM_FADERS
#define NUM_FADERS 1
#endif


struct Fader {
  int _usable_min;
  int _usable_max;
};

struct Calib {
  Fader fader[NUM_FADERS]; // par exemple 4 faders
};

extern Calib gCalib;       // variable globale partagée


void calibInit();          // à appeler dans setup()