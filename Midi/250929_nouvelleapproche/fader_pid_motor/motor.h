#pragma once
#include <cstdint>           // pour uint8_t
#include "fader_filtre_adc.h"
#include "debug.h"    // DBUG

// ===================== RÉGLAGES (tout en haut) =====================
constexpr uint8_t MAX_MOTOR = MAX_FADERS;   // limite dure (ne pas dépasser)
constexpr uint8_t NUM_MOTOR = NUM_FADERS;   // ← règle ici (1..11)
constexpr uint32_t freqMotor = 25000; // 25 kHz fréquence PWM moteur

struct Motor {
    uint8_t _in1;
    uint8_t _in2;
};

// Liste des broches de contrôle des moteurs (IN1, IN2)
extern Motor motors[MAX_MOTOR];

// ===================== RÉGLAGES  motor =====================
constexpr float breakv = 0.83f ; // limite les action à 10v idéal pour le moteur 
constexpr uint8_t FREIN_ACTIF_CYCLES = 4; // nombre de cycles pour activer le frein

extern uint8_t freinActifCount[NUM_MOTOR];

// ===================== API =====================
void setupmotor();
void loopmotor(uint8_t i);
