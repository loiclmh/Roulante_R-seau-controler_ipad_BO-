// bash_test.hpp
#pragma once
#include <Arduino.h>
#include "fader_filtre_adc.h" // pour setPosition[] et ADC_MAX
#include "pid.h"   // pour kp_python, ki_python, kd_python, ts_python, fc_python
#include "motor.h" // pour NUM_MOTOR
#include "debug.hpp" // pour on_debug, on_debug_python, on_debug_monitorarduino

// === Position injectée par Python ===
// - Soit en pas ADC (0..ADC_MAX) via commande POS
// - Soit en pourcentage (0..100) via commande POS%
// Choisis l’une ou l’autre dans handleSerialPython()
inline uint16_t pos_python_adc = ADC_MAX / 2;  // par défaut au milieu

namespace BashTestLocal {
  inline constexpr uint8_t kNumSteps = 6;
  inline const uint8_t kStepsPct[kNumSteps] = {5, 50, 25, 95, 75, 0};
  inline uint16_t kStepsADC[kNumSteps];

  inline uint8_t  stepIndex = 0;        // étape en cours
  inline uint32_t nextAtMs  = 0;        // prochain changement de position
  inline uint8_t  currentMotor = 0;     // moteur en cours de test
}


// =========================
//        TEST LOCAL
// =========================
//

void loop_test_bash_local() {
  if (bash_test_mode != 1) return;

  const uint32_t now = millis();

  // si c'est l'heure de passer à l'étape suivante
  if ((int32_t)(now - BashTestLocal::nextAtMs) >= 0) {
    setPosition[BashTestLocal::currentMotor] =
        BashTestLocal::kStepsADC[BashTestLocal::stepIndex];

    // étape suivante
    BashTestLocal::stepIndex++;

    if (BashTestLocal::stepIndex >= BashTestLocal::kNumSteps) {
      // on a fini toutes les étapes pour ce moteur
      BashTestLocal::stepIndex = 0;
      BashTestLocal::currentMotor++;

      if (BashTestLocal::currentMotor >= NUM_MOTOR) {
        // tous les moteurs ont été testés → on arrête
        bash_test_mode = 0;
        return;
      }
    }

    BashTestLocal::nextAtMs = now + 500; // 500 ms entre étapes
  }

  // on met à jour uniquement le moteur en cours
  loopPID(BashTestLocal::currentMotor);
}

