#pragma once
#include <Arduino.h>

// Default pins (can be changed in setup)
static const uint8_t PIN_DRV8871_PWM = 16; // GP16
static const uint8_t PIN_DRV8871_DIR = 17; // GP17
static const uint8_t PIN_FADER_ADC   = 26; // ADC0
static const uint8_t PIN_BUTTON_NOISE= 18;  // GP2 (active LOW)
static const uint8_t LED_PIN         = 25; // onboard
