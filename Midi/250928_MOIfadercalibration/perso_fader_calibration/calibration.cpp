#include "calibration.h"

#ifndef TEST
#define TEST 0
#endif

// --- Profil TEST : valeurs fournies par un script Python ---
// Si le fichier auto-généré existe, on l'utilise. Sinon on garde un fallback par défaut.
#if TEST == 1
  #if __has_include("calibration_test_autogen.h")
    #include "calibration_test_autogen.h" // doit fournir: extern const Fader CALIB_TEST[NUM_FADERS];
  #else
    static const Fader CALIB_TEST[NUM_FADERS] = {
        {100, 4000}, // Fader 0 (fallback)
        
    };
  #endif
#endif

// Exemple de valeurs pour les 4 faders en mode normal
static const Fader CALIB_NORMAL[NUM_FADERS] = {
    {100, 4025},
    
};

// Facteur de lissage du filtre Control Surface (3 = plus réactif, 5/6 = plus doux)
constexpr uint8_t FILTER_SHIFT = 4;

Calib gCalib;

void calibInit() {
#if TEST == 1
  const Fader* src = CALIB_TEST;   // Python/autogen ou fallback
#else
  const Fader* src = CALIB_NORMAL; // valeurs normales
#endif
  for (int i = 0; i < NUM_FADERS; ++i) {
    gCalib.fader[i]._usable_min = src[i]._usable_min;
    gCalib.fader[i]._usable_max = src[i]._usable_max;
  }
}