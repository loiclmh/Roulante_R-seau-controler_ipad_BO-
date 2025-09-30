#pragma once
// OLED I2C (Raspberry Pi Pico - remap GP4/GP5)
constexpr int PIN_I2C_SDA = 4;
constexpr int PIN_I2C_SCL = 5;
// DRV8871
constexpr int PIN_MOTOR_IN1 = 16;
constexpr int PIN_MOTOR_IN2 = 17;
// Fader ADC (GP26 = ADC0)
constexpr int PIN_FADER_ADC = 26;
// OLED infos
constexpr int OLED_WIDTH  = 128;
constexpr int OLED_HEIGHT = 64;
constexpr int OLED_ADDR_1 = 0x3C;
constexpr int OLED_ADDR_2 = 0x3D;
