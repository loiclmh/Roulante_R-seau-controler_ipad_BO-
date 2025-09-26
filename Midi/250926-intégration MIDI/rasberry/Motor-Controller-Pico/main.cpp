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
#define FORCE_SPIN_TEST 0  // 1 = force moteur à tourner pour debug, 0 = fader->moteur

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
static uint32_t expStartMs = 0;  // timestamp when we received 's'

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
    expStartMs = millis();
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
#if OPEN_LOOP_TEST
  #if FORCE_SPIN_TEST
    Display::showSplash("Motor forced test (+/-)\nOpen-loop");
  #else
    Display::showSplash("Fader -> Motor (open-loop)");
  #endif
#else
  Display::showSplash("Waiting 's' from PC");
#endif
  Motor::begin();

  controller.setTunings(6.0f, 2.0f, 0.035f, 60.0f);
  controller.reset();

  analogReadResolution(12); // RP2040 ADC 0..4095
}

void loop() {
#if OPEN_LOOP_TEST
  // ==== Mode test câblage (désactivé) ====
  if (tick1k()) {
#if !FORCE_SPIN_TEST
    uint16_t raw12 = analogRead(PIN_FADER_WIPER);   // 0..4095
    uint16_t y10   = raw12 >> 2;                    // 0..1023
    static float ema = 0.0f;
    ema = 0.5f * y10 + 0.5f * ema;                  // lissage plus réactif

    // map 0..1023 -> -1000..+1000 (centre ~512)
    int16_t u = map((int)ema, 0, 1023, -1000, 1000);
    Motor::drive(u);

    // UI ~20 Hz
    static uint32_t t_ui = 0;
    uint32_t now = millis();
    if (now - t_ui >= 50) {
      t_ui = now;
      Display::showFaderAndMotor((uint16_t)ema, y10, u);
    }
#else
    // ===== TEST FORCÉ MOTEUR : alterne +600 / -600 toutes les 2 s =====
    static uint32_t t0 = millis();
    uint32_t t = (millis() - t0) / 2000; // tranche de 2 s
    int16_t u = (t % 2 == 0) ? +600 : -600;  // force un couple non nul
    Motor::drive(u);

    // UI ~20 Hz
    static uint32_t t_ui = 0;
    uint32_t now = millis();
    if (now - t_ui >= 50) {
      t_ui = now;
      // On affiche des valeurs synthétiques pour debug
      Display::showFaderAndMotor( (t%2)?900:100, (t%2)?900:100, u );
    }
#endif
  }
#else
  // ==== Mode expérience PID + profil + SLIP ====
  // 1) Lire les paquets SLIP entrants
  if (rx.poll()) {
    handlePacket(rx.buf, rx.len);
    rx.len = 0;  // <-- consommer le paquet après traitement
  }

  // 2) Boucle 1 kHz
  if (tick1k()) {
    if (experimentOn) {
      // Pendant les 500 premières ms, envoie un heartbeat
      if ((uint32_t)(millis() - expStartMs) < 500) {
        uint8_t payload[6] = {0,0,0,0,0,0};
        slipWritePacket(payload, sizeof(payload));
        HALSerial::flush();
        static bool led = false; led = !led; digitalWrite(PIN_DEBUG_LED, led);
      } else {
        uint16_t r = RefProfile::next();
        uint16_t raw12 = analogRead(PIN_FADER_WIPER);
        uint16_t y     = raw12 >> 2;
        int16_t u = controller.update((int16_t)r, (int16_t)y);
        Motor::drive(u);

        uint8_t payload[6];
        put_i16_le(&payload[0], (int16_t)r);
        put_i16_le(&payload[2], (int16_t)y);
        put_i16_le(&payload[4], (int16_t)(u * 30));
        slipWritePacket(payload, sizeof(payload));
        HALSerial::flush();
        txCount++;
      }

      if (RefProfile::finished()) {
        experimentOn = false;
        Motor::stop();
      }
    }
  }
#endif
}
