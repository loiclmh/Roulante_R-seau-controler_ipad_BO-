// === CONFIG (constants at top) ===
#include <Arduino.h>

const uint8_t PIN_IN1 = 16;   // DRV8871 IN1 (PWM A)
const uint8_t PIN_IN2 = 17;   // DRV8871 IN2 (PWM B)
const uint8_t PIN_ADC = 26;   // Fader position pot (GPIO26 / A0 on Pico)

const uint16_t USABLE_MIN = 40;    // sensor low clamp (tune)
const uint16_t USABLE_MAX = 4025;  // sensor high clamp (tune)
const uint16_t HYST       = 2;    // deadband to avoid buzz
const float    DUTY_MIN   = 0.16f; // minimum duty to overcome friction
const float    DUTY_MAX   = 0.60f; // max duty to limit current/noise
const uint32_t HOLD_OFF_MS= 20;    // delay before coast when in-band
const uint32_t FREQ       = 25000; // 25 kHz to avoid audible whine

// PWM resolution/range (Arduino-Pico supports setting this)
const uint32_t PWM_RANGE  = 65535; // 16-bit range for smoothness
const bool     INVERT_DIR = true;   // invert direction if needed (based on your logs)
const uint32_t SETTLE_MS  = 500;    // delay before logging (non-blocking)

// === UTILS ===
static inline uint16_t clampU16(uint16_t v, uint16_t lo, uint16_t hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

uint16_t read_pos() {
  // simple moving average to smooth ADC
  const int N = 8;
  uint32_t acc = 0;
  for (int i = 0; i < N; i++) {
    acc += analogRead(PIN_ADC);
    delayMicroseconds(100);
  }
  return (uint16_t)(acc / N);
}

void stopMotor() {
  analogWrite(PIN_IN1, 0);
  analogWrite(PIN_IN2, 0);
}

void drive(int32_t error) {
  // Compute duty from error magnitude
  float mag = (float)abs(error) / (float)(USABLE_MAX - USABLE_MIN);
  float duty = DUTY_MIN + mag;
  if (duty > DUTY_MAX) duty = DUTY_MAX;

  // Decide whether we need to increase the ADC value (move up) or decrease it
  bool needIncrease = (error > 0); // target > pos => increase
  if (INVERT_DIR) needIncrease = !needIncrease; // flip if wiring is reversed

  if (needIncrease) {
    analogWrite(PIN_IN2, 0);
    analogWrite(PIN_IN1, (uint32_t)(duty * PWM_RANGE));
  } else {
    analogWrite(PIN_IN1, 0);
    analogWrite(PIN_IN2, (uint32_t)(duty * PWM_RANGE));
  }
}

// === RUNTIME ===
uint16_t target_raw = (USABLE_MIN + USABLE_MAX) / 2;
uint32_t stateStartMs = 0;
int state = 0; // 0=center,1=top,2=bottom
uint32_t lastInBandMs = 0;
uint32_t nextLogMs = 0; // timestamp to print target/position after a settle delay

void setup() {
  // ADC setup: use 12-bit reads on Pico core (0..4095)
  analogReadResolution(12);

  // PWM setup
  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);

  // On Arduino-Pico core, these affect PWM on all pins
  analogWriteRange(PWM_RANGE);
  analogWriteFreq(FREQ); // set PWM frequency to ultrasonic range

  // Start stopped
  stopMotor();
  Serial.begin(115200);
}

void loop() {
  if (millis() - stateStartMs > 2000) {
    state = (state + 1) % 3;
    if (state == 0) target_raw = (USABLE_MIN + USABLE_MAX) / 2;
    if (state == 1) target_raw = USABLE_MAX;
    if (state == 2) target_raw = USABLE_MIN;
    stateStartMs = millis();
    nextLogMs = stateStartMs + SETTLE_MS; // schedule a log 0.5s later (non-blocking)
  }

  uint16_t pos = read_pos();
  pos = clampU16(pos, USABLE_MIN, USABLE_MAX);
  int32_t err = (int32_t)target_raw - (int32_t)pos;

  if (err > HYST) {
    drive(err);
    lastInBandMs = millis();
  } else if (err < -(int32_t)HYST) {
    drive(err);
    lastInBandMs = millis();
  } else {
    // In the neutral band: coast after HOLD_OFF_MS for silence
    if (millis() - lastInBandMs > HOLD_OFF_MS) {
      stopMotor();
    }
  }

  // Non-blocking print ~0.5s after changing target
  if (nextLogMs && (int32_t)(millis() - nextLogMs) >= 0) {
    uint16_t posNow = read_pos();
    Serial.print("Target: ");
    Serial.print(target_raw);
    Serial.print(" | Position: ");
    Serial.println(posNow);
    nextLogMs = 0; // reset
  }

  delay(1); // small loop delay
}