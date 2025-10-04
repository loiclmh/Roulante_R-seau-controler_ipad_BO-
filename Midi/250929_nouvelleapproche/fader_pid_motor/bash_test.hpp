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


// =========================
//   PARSER SÉRIE PYTHON
// =========================
//
// Commandes texte (une par ligne) à envoyer sur le port série :
//
// - PID <kp> <ki> <kd> <ts> <fc>
//     Ex: "PID 1.00 0.20 0.05 0.001 10"
//
// - POS <i> <valADC>           (position en pas ADC, 0..ADC_MAX)
//     Ex: "POS 0 2048"
//
// - POS% <i> <pct>             (position en %, 0..100)
//     Ex: "POS% 2 75"
//
// - MODE <m>                   (0=OFF, 1=LOCAL, 2=PYTHON)
//     Ex: "MODE 1"
//
// Réponses : "OK", "ERR ...".
//
void setup_python() {
    if (!Serial.available()) return;

    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) return;

    // PID kp ki kd ts fc
    if (line.startsWith("PID")) {
      float Kp, Ki, Kd, Ts, Fc;
      if (sscanf(line.c_str(), "PID %f %f %f %f %f", &Kp, &Ki, &Kd, &Ts, &Fc) == 5) {
        kp_python = Kp;
        ki_python = Ki;
        kd_python = Kd;
        ts_python = Ts;
        fc_python = Fc;
        Serial.println(F("OK PID"));
      } else {
        Serial.println(F("ERR PID parse"));
      }
      return;
    }

    // POS i valADC
    if (line.startsWith("POS ")) {
      int i; long v;
      if (sscanf(line.c_str(), "POS %d %ld", &i, &v) == 2) {
        if ((uint8_t)i < NUM_MOTOR && v >= 0 && v <= ADC_MAX) {
          pos_python_adc = (uint16_t)v;
          setPosition[i] = (uint16_t)v;
          Serial.println(F("OK POS"));
        } else {
          Serial.println(F("ERR POS range"));
        }
      } else {
        Serial.println(F("ERR POS parse"));
      }
      return;
    }

    // POS% i pct
    if (line.startsWith("POS%")) {
      int i; int pct;
      if (sscanf(line.c_str(), "POS%% %d %d", &i, &pct) == 2) {
        if ((uint8_t)i < NUM_MOTOR && pct >= 0 && pct <= 100) {
          uint16_t val = (uint16_t)((uint32_t)pct * ADC_MAX / 100u);
          pos_python_adc = val;
          setPosition[i] = val;
          Serial.println(F("OK POS%"));
        } else {
          Serial.println(F("ERR POS% range"));
        }
      } else {
        Serial.println(F("ERR POS% parse"));
      }
      return;
    }

    // MODE m
    if (line.startsWith("MODE")) {
      int m;
      if (sscanf(line.c_str(), "MODE %d", &m) == 1) {
        if (m >= 0 && m <= 2) {
          bash_test_mode = (uint8_t)m;
          Serial.println(F("OK MODE"));
        } else {
          Serial.println(F("ERR MODE range"));
        }
      } else {
        Serial.println(F("ERR MODE parse"));
      }
      return;
    }

    Serial.println(F("ERR cmd"));
  } 
}

