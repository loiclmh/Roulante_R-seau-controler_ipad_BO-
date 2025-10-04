// ========================== bash_tets_python.hpp ==========================
#pragma once
#include <Arduino.h>
#include <string.h>     // memcpy
#include <ctype.h>      // isdigit

#include "fader_filtre_adc.h"  // <-- pour MAX_FADERS / NUM_FADERS
#include "motor.h"             // <-- pour NUM_MOTOR (dépend de fader_filtre_adc.h)
#include "pid.h"               // <-- pour gPid[] et la classe PID (méthodes visibles ici

// ---------- Externs (définis ailleurs dans ton projet) ----------
extern PID*     gPid[NUM_MOTOR];
extern uint16_t setPosition[NUM_MOTOR];
extern uint16_t gFaderADC[MAX_FADERS];
extern int16_t  Dirmotor[NUM_MOTOR];

extern float kp_python, ki_python, kd_python, ts_python, fc_python;
extern void  initial_PIDv(bool use_python);

extern uint8_t bash_test_mode; // si tu veux déclencher le test local

// ====================== SLIP encoder/decoder =========================
static constexpr uint8_t SLIP_END     = 0xC0;
static constexpr uint8_t SLIP_ESC     = 0xDB;
static constexpr uint8_t SLIP_ESC_END = 0xDC;
static constexpr uint8_t SLIP_ESC_ESC = 0xDD;

inline void slipWriteByte(uint8_t b) {
  if (b == SLIP_END)      { Serial.write(SLIP_ESC); Serial.write(SLIP_ESC_END); }
  else if (b == SLIP_ESC) { Serial.write(SLIP_ESC); Serial.write(SLIP_ESC_ESC); }
  else                    { Serial.write(b); }
}

inline void slipWrite(const uint8_t* data, size_t len) {
  Serial.write(SLIP_END);
  for (size_t i = 0; i < len; ++i) slipWriteByte(data[i]);
  Serial.write(SLIP_END);
}

// Envoie un tableau de float32 en SLIP
inline void slipWriteFloats(const float* f, size_t n) {
  Serial.write(SLIP_END);
  for (size_t i = 0; i < n; ++i) {
    uint8_t b[4];
    memcpy(b, &f[i], 4);    // RP2040 = little-endian → direct
    for (uint8_t k : b) slipWriteByte(k);
  }
  Serial.write(SLIP_END);
}

// RX buffer
inline uint8_t  rxBuf[128];
inline uint16_t rxLen = 0;
inline bool     rxEsc = false;

inline void slipFeed(uint8_t b, void (*onPacket)(const uint8_t*, uint16_t)) {
  if (b == SLIP_END) {
    if (rxLen > 0 && onPacket) onPacket(rxBuf, rxLen);
    rxLen = 0; rxEsc = false; return;
  }
  if (b == SLIP_ESC) { rxEsc = true; return; }
  if (rxEsc) {
    if      (b == SLIP_ESC_END) b = SLIP_END;
    else if (b == SLIP_ESC_ESC) b = SLIP_ESC;
    rxEsc = false;
  }
  if (rxLen < sizeof rxBuf) rxBuf[rxLen++] = b;
}

// ======================== Commandes Python =====================
// Format: b'<cmd><idx_ascii><float32>'
// cmd ∈ { 'p','i','d','t','c','s' }
inline bool parseIdxAndValue(const uint8_t* data, uint16_t len, uint8_t& idx, float& val) {
  if (len < 2) return false;
  uint16_t p = 1;
  uint32_t idx_acc = 0;
  while (p < len && isdigit(data[p])) {
    idx_acc = idx_acc * 10 + (data[p] - '0');
    ++p;
  }
  if (p + 4 > len) return false; // besoin 4 octets float
  idx = (uint8_t)idx_acc;
  memcpy(&val, data + p, 4);     // float32 LE
  return true;
}

inline void onSlipPacket(const uint8_t* data, uint16_t len) {
  if (len < 2) return;
  const char cmd = (char)data[0];
  uint8_t idx = 0;
  float   v   = 0.f;
  if (!parseIdxAndValue(data, len, idx, v)) return;
  if (idx >= NUM_MOTOR) return;

  switch (cmd) {
    case 'p':
      kp_python = v;
      if (gPid[idx]) gPid[idx]->setKp(kp_python);
      break;
    case 'i':
      ki_python = v;
      if (gPid[idx]) gPid[idx]->setKi(ki_python);
      break;
    case 'd':
      kd_python = v;
      if (gPid[idx]) gPid[idx]->setKd(kd_python);
      break;
    case 'c':
      fc_python = v;
      if (gPid[idx]) gPid[idx]->setEMACutoff(fc_python);
      break;
    case 't':
      ts_python = v;
      initial_PIDv(true); // recrée les PID avec Ts mis à jour
      break;
    case 's':
      bash_test_mode = 1; // démarrer profil local si tu veux
      break;
    default: break;
  }
}

// ============================ API côté sketch ========================
inline void tuningBegin(unsigned long baud = 1000000) {
  Serial.begin(baud);
  unsigned long t0 = millis();
  while (!Serial && (millis() - t0) < 1500) {}
}

// À appeler souvent dans loop() pour traiter les commandes Python
inline void tuningHandle() {
  while (Serial.available()) {
    slipFeed((uint8_t)Serial.read(), onSlipPacket);
  }
}

// Envoi 1 échantillon [t, setpoint, meas, u]
inline void tuningSendSample(uint8_t i, float seconds, float setp, float meas, float u) {
  float v[4] = { seconds, setp, meas, u };
  slipWriteFloats(v, 4);
}