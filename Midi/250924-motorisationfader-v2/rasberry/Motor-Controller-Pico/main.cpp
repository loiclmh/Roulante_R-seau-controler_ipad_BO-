#if __has_include(<Adafruit_TinyUSB.h>)
  #include <Adafruit_TinyUSB.h>
#endif

#include <Arduino.h>
#include "pins.h"
#include "hal_serial.h"
#include "display.h"
#include "motor.hpp"
#include "ref_profile.hpp"
#include "slip.h"
#include "pid.h"

// ====== Mode ======
// 0 = expérience PID + profil + SLIP (avec Tuning.py)
// 1 = test open-loop (fader -> moteur direct) [désactivé ici]
#define OPEN_LOOP_TEST 0

// ------- Tick 1 kHz -------
static inline bool tick1k() {
  static uint32_t last = 0;
  uint32_t now = micros();
  if ((uint32_t)(now - last) >= 1000) { last += 1000; return true; }
  return false;
}

// ------- Helpers LE (pour Tuning.py : int16 little-endian) -------
static inline void put_i16_le(uint8_t* dst, int16_t v) {
  dst[0] = (uint8_t)(v & 0xFF);
  dst[1] = (uint8_t)((v >> 8) & 0xFF);
}

// ------- État appli -------
static SlipDecoder rx;            // décodeur SLIP entrant
static bool experimentOn = false; // activé quand on reçoit 's'
static uint32_t txCount = 0;

// ------- PID -------
static PID controller;

// ------- Handler SLIP -------
// 'p','i','d','c' + float32 LE : réglages PID
// 's' + float32 : start (speed_div côté Python — ignoré ici)
static void handlePacket(const uint8_t* p, size_t n) {
  if (n < 2) return;
  const char cmd = (char)p[0];
  const char faderIdx = (char)p[1];
  (void)faderIdx; // un seul fader: '0'

  if (cmd == 's') {
    experimentOn = true;
    txCount = 0;
    controller.reset();
    RefProfile::reset();
    Display::showSplash("Start exp OK");

    // Prime: quelques trames neutres pour que Python accroche
    for (int k = 0; k < 20; ++k) {
      uint8_t payload[6] = {0,0,0,0,0,0};
      slipWritePacket(payload, sizeof(payload));
      HALSerial::flush();
      delay(2);
    }
    return;
  }

  // Réception d'un réglage PID
  if (n == 6) {
    float val;
    memcpy(&val, &p[2], sizeof(float));   // float32 LE
    static float Kp=6, Ki=2, Kd=0.035f, fc=60;
    switch (cmd) {
      case 'p': Kp = val; break;
      case 'i': Ki = val; break;
      case 'd': Kd = val; break;
      case 'c': fc = val; break;
      default: break;
    }
    controller.setTunings(Kp, Ki, Kd, fc);
  }
}

void setup() {
  pinsBegin();
  HALSerial::begin();
  Display::begin();
  Display::showSplash("Waiting 's' from PC");
  Motor::begin();

  controller.setTunings(6.0f, 2.0f, 0.035f, 60.0f);
  controller.reset();

  analogReadResolution(12); // RP2040 ADC 0..4095
}

void loop() {
#if OPEN_LOOP_TEST
  // ==== Mode test câblage (désactivé) ====
  if (tick1k()) {
    uint16_t raw12 = analogRead(PIN_FADER_WIPER);
    uint16_t y10   = raw12 >> 2;
    static float ema = 0.0f;
    ema = 0.3f * y10 + 0.7f * ema;

    int16_t u = map((int)ema, 0, 1023, -1000, 1000);
    Motor::drive(u);

    static uint32_t t_ui = 0;
    uint32_t now = millis();
    if (now - t_ui >= 50) {
      t_ui = now;
      Display::showFaderAndMotor((uint16_t)ema, y10, u);
    }
  }
#else
  // ==== Mode expérience PID + profil + SLIP ====
  // 1) Lire les paquets SLIP entrants
  if (rx.poll()) {
    handlePacket(rx.buf, rx.len);
  }

  // 2) Boucle 1 kHz
  if (tick1k()) {
    if (experimentOn) {
      // Référence (0..1023) type TTTapa
      uint16_t r = RefProfile::next();

      // Mesure fader (ADC 12 -> 10 bits)
      uint16_t raw12 = analogRead(PIN_FADER_WIPER);
      uint16_t y     = raw12 >> 2;

      // PID ~ [-1000 ; +1000]
      int16_t u = controller.update((int16_t)r, (int16_t)y);

      // Pilotage DRV8871
      Motor::drive(u);

      // Envoi (ref, pos, u) vers Python
      uint8_t payload[6];
      put_i16_le(&payload[0], (int16_t)r);
      put_i16_le(&payload[2], (int16_t)y);
      put_i16_le(&payload[4], (int16_t)(u * 30)); // facteur pour visibilité
      slipWritePacket(payload, sizeof(payload));
      HALSerial::flush();
      txCount++;

      // Fin d'essai -> coupe envoi + moteur
      if (RefProfile::finished()) {
        experimentOn = false;
        Motor::stop();
      }
    }
  }
#endif
}
