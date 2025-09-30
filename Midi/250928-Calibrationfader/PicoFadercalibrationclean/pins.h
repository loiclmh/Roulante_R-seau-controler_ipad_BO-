#pragma once
// === Pinout (Raspberry Pi Pico / RP2040) ===
// Adjust if you wire differently.
constexpr int PIN_FADER = 26;      // ADC0 = GP26
constexpr int PIN_I2C_SDA = 4;    // GP16
constexpr int PIN_I2C_SCL = 5;    // GP17

// === OLED ===
constexpr int OLED_WIDTH  = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_ADDR_1 = 0x3C;
constexpr int OLED_ADDR_2 = 0x3D;
