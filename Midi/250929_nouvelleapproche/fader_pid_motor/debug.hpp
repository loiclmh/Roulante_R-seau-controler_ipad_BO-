#pragma once
#include <cstdint>

// ================================= variable pour débug programe =========================

// variable pour activer les print de debug dans les fichiers .cpp
inline bool on_debug = true ; // active ou non les print dans pid.cpp
inline bool on_debug_python = true ; // active ou non les print dans motor.cpp
inline bool on_debug_monitorarduino = true ; // active ou non les print dans bash_test.cpp

// ======================== variable pour le mode test =========================
// 0 = OFF (rien), 1 = TEST LOCAL (séquence 5-50-25-95-75-0), 2 = PYTHON (réglages via série)
inline uint8_t bash_test_mode = 0; 

//====================== DEBUG FADER ADC =====================
inline int debugOLED_fader = 1; // 0=off, 1=affiche valeurs ADC dans le moniteur série
inline int debug_fadermoniteur = 0; // 0=off, 1=affiche ready sur le moniteur 















inline void debugsetup () {

   if (on_debug == true) return ; 
    else {
    on_debug_python = false ;
    on_debug_monitorarduino = false ;
    debugOLED_fader = 0;
    debug_fadermoniteur = 0;
    }
}
