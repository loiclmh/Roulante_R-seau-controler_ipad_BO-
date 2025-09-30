#pragma once
#include <Arduino.h>
#include <Control_Surface.h>   // lib Arduino Control Surface
#include "pins.h"

// =================== Réglages MIDI (par défaut) ===================
static uint8_t MIDI_CC_NUM = 16;        // CC par défaut
static Channel MIDI_CH      = Channel_1; // Canal par défaut (1..16)

// =============== Interface MIDI USB (Control_Surface) ==============
static USBMIDI_Interface midi;          // RP2040 + Control_Surface requis

// Réception de la consigne (0..127)
static CCValue ccIn{ MIDIAddress{MIDI_CC_NUM, MIDI_CH} };

// Mémo pour éviter de spammer en émission
static uint8_t  lastSentCC  = 0xFF; // force envoi au 1er tick
static uint32_t lastRxMs    = 0;    // horodatage dernier RX (badge MIDI)

// --------------------- Helpers de config ---------------------------
inline void MIDIIO_config(uint8_t ccNumber, Channel ch) {
  // Met à jour le numéro de CC et le canal à chaud (si besoin)
  MIDI_CC_NUM = ccNumber;
  MIDI_CH     = ch;
  // Rebind l'objet CCValue sur la nouvelle adresse
  ccIn = CCValue{ MIDIAddress{MIDI_CC_NUM, MIDI_CH} };
}

inline void MIDIIO_begin() {
  Control_Surface.begin();  // init USB MIDI
}

// Convertit ADC [0..1023] -> CC [0..127]
inline uint8_t adcToCC(uint16_t y10) {
  if (y10 > 1023) y10 = 1023;
  return (uint8_t) map((int)y10, 0, 1023, 0, 127);
}
// Convertit CC [0..127] -> ref ADC [0..1023]
inline uint16_t ccToAdc(uint8_t cc) {
  if (cc > 127) cc = 127;
  return (uint16_t) map((int)cc, 0, 127, 0, 1023);
}

// ---------------------- Boucle principale --------------------------
inline void MIDIIO_loop() {
  Control_Surface.loop();
}

// Lit une consigne CC reçue → refOut (0..1023). Rend true si MAJ.
inline bool MIDIIO_getTargetIfUpdated(uint16_t &refOut) {
  if (ccIn.getDirty()) {
    uint8_t v = ccIn.getValue();
    refOut = ccToAdc(v);
    ccIn.clearDirty();
    lastRxMs = millis();        // mémorise le moment de réception
    return true;
  }
  return false;
}

// Indique s'il y a eu du RX récent (pour badge OLED)
inline bool MIDIIO_hasRecentRX(uint32_t window_ms = 300) {
  return (millis() - lastRxMs) < window_ms;
}

// Envoie la position courante (avec hystérésis LSB) pendant le toucher
inline void MIDIIO_sendPositionFromADC(uint16_t y10, uint8_t hysteresisLSB = 1) {
  uint8_t cc = adcToCC(y10);
  if (lastSentCC == 0xFF || (abs((int)cc - (int)lastSentCC) >= hysteresisLSB)) {
    midi.sendControlChange(MIDIAddress{MIDI_CC_NUM, MIDI_CH}, cc);
    lastSentCC = cc;
  }
}