#pragma once
#include <Arduino.h>

// API minimal DRV8871 (IN1/IN2)
void motor_begin(uint8_t in1, uint8_t in2, uint32_t pwm_hz);
void motor_drive(int16_t cmd255); // -255..+255 ; 0 -> coast
void motor_brake();               // HIGH/HIGH
void motor_coast();               // LOW/LOW
