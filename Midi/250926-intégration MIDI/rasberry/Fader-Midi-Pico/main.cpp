#include "pins.h"
#include "display.h"
#include "motor.hpp"
#include "ref_profile.hpp"
#include "touch.hpp"
#include "midi_io.hpp"
#include <Arduino.h>

// ==== Support Adafruit TinyUSB ====
// - Avec le core RP2040 (Earle Philhower), choisis dans l'IDE :
//   Tools → USB Stack → Adafruit TinyUSB
// - Le port série "Serial" est alors fourni par TinyUSB (CDC).
// - Si la lib <Adafruit_TinyUSB.h> est dispo, on l'inclut (facultatif).
#if __has_include(<Adafruit_TinyUSB.h>)
  #include <Adafruit_TinyUSB.h>
#endif

namespace HALSerial {

  inline void begin(uint32_t /*baud*/ = 115200) {
    // Sous TinyUSB, le baud est ignoré côté device.
    Serial.begin(115200);
    // Attendre le montage USB (non bloquant trop longtemps)
    uint32_t t0 = millis();
    while (!Serial && (millis() - t0) < 1500) {
      delay(10);
    }
  }

  inline int available() {
    return Serial.available();
  }

  inline int read() {
    return Serial.read();
  }

  inline size_t write(uint8_t b) {
    return Serial.write(b);
  }

  inline void flush() {
    Serial.flush();
  }
}

void setup() {
  pinsBegin();
  HALSerial::begin();
  Display::begin();
  Touch_begin();          // init capteur tactile (GP2 + 1M -> 3V3, C optionnel -> GND)
  MIDIIO_begin();         // init USB MIDI (Control_Surface)
  Motor::begin();
  analogReadResolution(12); // ADC on RP2040
}

void loop() {
  static uint32_t last_us = 0;
  uint32_t now = micros();
  if ((uint32_t)(now - last_us) >= 1000) { // 1kHz loop
    last_us += 1000;
    // --- MIDI polling & reference capture ---
    static uint16_t ref10 = 0;        // consigne 0..1023 depuis le DAW
    static uint32_t midiBlinkMs = 0;  // pour badge MIDI
    bool midi_rx = false;

    MIDIIO_loop();
    if (MIDIIO_getTargetIfUpdated(ref10)) {
      midi_rx = true;
      midiBlinkMs = millis();
    } else {
      midi_rx = (millis() - midiBlinkMs) < 300;  // badge 300 ms
    }

    uint16_t raw12 = analogRead(PIN_FADER_WIPER); // 0..4095
    uint16_t y10   = raw12 >> 2; // 0..1023
    static float ema = 0.0f;
    ema = 0.3f * y10 + 0.7f * ema;

    // --- Priorité tactile & commande moteur ---
    const bool touched = Touch_isTouched();

    // Position fader filtrée -> ema (0..1023)
    // Consigne MIDI -> ref10 (0..1023)

    int16_t u = 0;
    if (touched) {
      Motor::stop();
      // Écriture vers le DAW pendant la prise en main
      MIDIIO_sendPositionFromADC((uint16_t)ema);
    } else {
      // Suivi simple P (test) : u = Kp * (ref - pos)
      int error = (int)ref10 - (int)ema;    // -1023..+1023
      const int Kp = 2;                     // gain de test
      long u32 = (long)error * Kp;
      if (u32 > 1000) u = 1000;
      else if (u32 < -1000) u = -1000;
      else u = (int16_t)u32;
      Motor::drive(u);
    }

    // UI refresh ~20Hz
    static uint32_t t_ui = 0;
    uint32_t now_ms = millis();
    if (now_ms - t_ui >= 50) {
      t_ui = now_ms;
      Display::showFaderAndMotor((uint16_t)ema, y10, u, touched, (int)ref10, midi_rx);
    }
  }
}
