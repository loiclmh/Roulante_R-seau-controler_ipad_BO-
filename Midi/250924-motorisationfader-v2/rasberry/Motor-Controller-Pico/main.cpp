// main.cpp
#if __has_include(<Adafruit_TinyUSB.h>)
  #include <Adafruit_TinyUSB.h> // ok même si USB Stack = Pico SDK
#endif

#include <Arduino.h>
#include "pins.h"
#include "hal_serial.h"
#include "slip.h"
#include "display.h"

// ------- Helpers LE (pour Tuning.py : int16 little-endian) -------
static inline void put_i16_le(uint8_t* dst, int16_t v) {
  dst[0] = (uint8_t)(v & 0xFF);
  dst[1] = (uint8_t)((v >> 8) & 0xFF);
}

// ------- État appli -------
static SlipDecoder rx;             // décodeur SLIP entrant
static bool     experimentOn = false; // passe à true après 's'
static uint32_t txCount      = 0;  // trames envoyées / seconde

// ------- Traitement d'un paquet SLIP entrant -------
// Format attendu (host -> Pico) : cmd(1B) + fader('0'..'3')(1B) + [float32 LE](4B) optionnel
static void handlePacket(const uint8_t* p, size_t n) {
  if (n < 2) return;
  const char cmd = (char)p[0];
  const char faderIdx = (char)p[1];
  (void)faderIdx; // pas utilisé pour l’instant

  switch (cmd) {
    case 's': // start experiment
      experimentOn = true;
      txCount = 0; // reset compteur
      Display::showMessage("Start exp (s)");
      break;

    // p/i/d/c/m viendront plus tard
    default:
      // Display::showMessage("Cmd recu");
      break;
  }
}

void setup() {
  pinsBegin();

  // OLED d'abord => feedback visuel immédiat
  Display::begin();
  Display::showMessage("Waiting 's' from PC");

  // USB CDC ensuite (non bloquant dans hal_serial.h)
  HALSerial::begin();

  digitalWrite(PIN_DEBUG_LED, LOW);
}

void loop() {
  // ----------- Réception SLIP ----------
  if (rx.poll()) {
    handlePacket(rx.buf, rx.len);
  }

  // ----------- Boucle fixe 1 kHz ----------
  static uint32_t last_us = 0;
  const uint32_t now = micros();
  if (now - last_us >= 1000) { // 1000 us = 1 kHz
    last_us += 1000;

    if (experimentOn) {
      // Données factices pour tester le flux (remplacées plus tard par ADC+PID)
      static int16_t t = 0;
      t += 20; // vitesse de défilement
      const float s = sinf(t * 0.005f);        // ref -1..1
      const float p = sinf(t * 0.005f + 0.5f); // pos simulée
      const float u = 0.6f * sinf(t * 0.005f + 1.0f); // commande simulée

      // Mise à l’échelle vers int16
      const int16_t ref_i16 = (int16_t)(s * 30000);
      const int16_t pos_i16 = (int16_t)(p * 30000);
      const int16_t u_i16   = (int16_t)(u * 30000);

      uint8_t payload[6];
      put_i16_le(&payload[0], ref_i16);
      put_i16_le(&payload[2], pos_i16);
      put_i16_le(&payload[4], u_i16);

      slipWritePacket(payload, sizeof(payload));
      HALSerial::flush();
      txCount++;

      // LED activité
      static bool led = false;
      led = !led;
      digitalWrite(PIN_DEBUG_LED, led);
    }
  }

  // ----------- Affichage OLED : TX/s toutes ~1 s ----------
  static uint32_t last_ms = 0;
  const uint32_t now_ms = millis();
  if (now_ms - last_ms >= 1000) {
    last_ms = now_ms;
    char buf[32];
    if (experimentOn) {
      snprintf(buf, sizeof(buf), "TX/s: %lu", (unsigned long)txCount);
    } else {
      snprintf(buf, sizeof(buf), "Waiting 's'...");
    }
    Display::showMessage(buf);
    txCount = 0;
  }
}
