#pragma once
#include <Arduino.h>

/*
  Raspberry Pi Pico (RP2040) – Mapping projet Fader

  ┌──────── Signal ───────┬──── GPIO ──┬──── Commentaires ────────────────────────────┐
  │ DRV8871_PWM          │   16       │ PWM vitesse moteur (IN1)                      │
  │ DRV8871_DIR          │   17       │ Direction moteur (IN2)                        │
  │ FADER_WIPER          │   26 (A0)  │ Lecture position (potentiomètre du fader)     │
  │ TOUCH_SENSE          │    2       │ Entrée détection tactile (RC timing)          │
  │ I2C_SDA (OLED)       │    4       │ SSD1306 SDA (I2C1)                            │
  │ I2C_SCL (OLED)       │    5       │ SSD1306 SCL (I2C1)                            │
  │ DEBUG_LED (option)   │   25       │ LED embarquée (Pico)                          │
  └───────────────────────┴────────────┴──────────────────────────────────────────────┘
*/

// === Motor driver DRV8871 ===
static constexpr uint8_t PIN_DRV8871_PWM = 16;   // IN1 (PWM)
static constexpr uint8_t PIN_DRV8871_DIR = 17;   // IN2 (DIR)

// === Fader ===
static constexpr uint8_t PIN_FADER_WIPER = A0;   // GPIO26 (ADC0)
static constexpr uint8_t PIN_TOUCH_SENSE = 2;    // à ajuster si besoin

// === OLED I2C (SSD1306 128x64) ===
static constexpr uint8_t PIN_I2C_SDA = 4;        // I2C1 SDA
static constexpr uint8_t PIN_I2C_SCL = 5;        // I2C1 SCL
static constexpr uint8_t OLED_I2C_ADDR = 0x3C;   // adresse la plus courante

// === Divers ===
static constexpr uint8_t PIN_DEBUG_LED = 25;     // LED intégrée

// Helper d'init (facultatif)
inline void pinsBegin() {
  pinMode(PIN_DRV8871_PWM, OUTPUT);
  pinMode(PIN_DRV8871_DIR, OUTPUT);
  digitalWrite(PIN_DRV8871_PWM, LOW);
  digitalWrite(PIN_DRV8871_DIR, LOW);

  pinMode(PIN_DEBUG_LED, OUTPUT);
  digitalWrite(PIN_DEBUG_LED, LOW);

  // I2C sera configuré dans display.cpp :
  // Wire.setSDA(PIN_I2C_SDA); Wire.setSCL(PIN_I2C_SCL);
}
