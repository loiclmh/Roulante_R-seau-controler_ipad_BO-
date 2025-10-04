#pragma once
#include "fader_filtre_adc.h"

// ===================== RÉGLAGES (tout en haut) =====================
constexpr uint8_t MAX_MOTOR = MAX_FADER ;   // limite dure (ne pas dépasser)
constexpr uint8_t NUM_MOTOR = NUM_FADER ;   // ← règle ici (1..11)
constexpr int freqMotor = 250000 ; // 25 kHz feq moteur 

struct Motor {
    uint8_t _in1;
    uint8_t _in2;
};

// Liste des broches de contrôle des moteurs (IN1, IN2)
Motor motors[NUM_MOTOR] = { 
    { 18, 17 }, // Motor 1
    { 16, 15 }, // Motor 2
    { 14, 13 }, // Motor 3
    { 12, 11 }  // Motor 4
};

// ===================== RÉGLAGES  motor =====================
constexpr float breakv = 0.83f ; // limite les action à 10v idéal pour le moteur 
constexpr uint8_t freinActif = 4; // nombre de cycle pour avoir un freinage actif désactivé

static uint8_t freinActif[NUM_MOTOR] = {0}; // valeur expérimental pour optimisé le frein

// ===================== API =====================
void setupmotor();
void loopmotor()

