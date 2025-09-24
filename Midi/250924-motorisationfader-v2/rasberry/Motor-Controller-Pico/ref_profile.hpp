#pragma once
#include <Arduino.h>

// Profil de test type TTTapa (rampe + paliers), implémenté inline
namespace RefProfile {
  static uint16_t ref      = 0;
  static uint32_t phase_t  = 0;
  static uint8_t  phase_id = 0;
  static bool     done     = false;

  inline void reset() { ref=0; phase_t=0; phase_id=0; done=false; }

  inline uint16_t next() {
    switch (phase_id) {
      case 0: if (++phase_t >= 500) { phase_t=0; phase_id=1; } return 0;
      case 1: {
        if (++phase_t <= 1200) {
          float x = phase_t / 1200.0f;
          ref = (uint16_t)(x * 1023.0f + 0.5f);
        } else { ref = 1023; phase_t=0; phase_id=2; }
        return ref;
      }
      case 2: if (++phase_t >= 200) { phase_t=0; phase_id=3; } return 1023;
      case 3: if (phase_t++ == 0) ref = 0; if (phase_t >= 300) { phase_t=0; phase_id=4; } return ref;
      case 4:
      default: {
        static const uint16_t steps[] = { 0, 350, 700, 350, 0, 500 };
        static const size_t   N = sizeof(steps)/sizeof(steps[0]);
        size_t idx = phase_t / 400;
        if (idx >= N) { done = true; return ref; }
        if ((phase_t % 400) == 0) ref = steps[idx];
        ++phase_t;
        return ref;
      }
    }
  }

  inline bool finished() { return done; }
}
