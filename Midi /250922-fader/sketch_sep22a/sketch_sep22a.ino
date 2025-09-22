/*
 * Raspberry Pi Pico — Motorized Fader + OLED menu
 * - TX: sends CC#16 on Channel 2 from POT (A0)
 * - RX: follows CC#16 on Channel 1 (feedback) to motorize fader (DRV8871 on GP14/GP15)
 * - OLED (I2C) shows Target/Pos + simple menu to tweak Kp, Ki, Deadband, MinPWM, Enable, Invert, and Source mode (Feedback/Local)
 */

#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- OLED ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDR    0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- MIDI ----------------
USBMIDI_Interface midi;
constexpr uint8_t CC_NUM_TX = 16;   // CC sent (POT -> iPad) on CH2
constexpr uint8_t CC_NUM_RX = 16;   // CC feedback (iPad -> motor) on CH1

// POT -> CC TX
constexpr uint8_t PIN_FADER_ADC = A0; // GP26
CCPotentiometer potOut {
  PIN_FADER_ADC,
  { CC_NUM_TX, Channel_2 },
};

// CC RX (feedback) -> motor target
CCValue ccIn{ MIDIAddress{ CC_NUM_RX, Channel_1 } };

// ------------- Motor (DRV8871) -------------
constexpr uint8_t PIN_IN1 = 14;  // GP14 (PWM)
constexpr uint8_t PIN_IN2 = 15;  // GP15 (PWM)

// ------------- Buttons (menu) -------------
constexpr uint8_t PIN_BTN_UP   = 16;  // GP16
constexpr uint8_t PIN_BTN_DOWN = 17;  // GP17
constexpr uint8_t PIN_BTN_OK   = 18;  // GP18

// ------------- Control params -------------
constexpr int   ADC_MAX   = 4095;
constexpr int   MAX_PWM   = 255;
int             deadband  = 6;
int             minPWM    = 25;
bool            motorEnabled = true;
bool            invertMotor  = false;

int   kp_x10   = 9;     // Kp = kp_x10 / 10.0
int   ki_x1000 = 10;    // Ki = ki_x1000 / 1000.0
float integral  = 0.0f;
int   targetADC = 0;

// -------- Source mode --------
enum class SourceMode : uint8_t { FEEDBACK = 0, LOCAL = 1 };
SourceMode sourceMode = SourceMode::FEEDBACK;
uint8_t    localValue = 64; // 0..127

// -------- Menu items --------
enum class MenuItem : uint8_t {
  SOURCE = 0,
  LOCAL_VALUE,
  KP,
  KI,
  DEADBAND,
  MIN_PWM,
  MOTOR_ENABLE,
  INVERT_MOTOR,
  _COUNT
};
MenuItem currentItem = MenuItem::SOURCE;

// --------- Button helper (declare BEFORE function) ---------
struct Btn {
  uint8_t pin;
  bool    state;        // stable level
  bool    lastStable;
  uint32_t lastChangeMs;
};

Btn btnUp   {PIN_BTN_UP,   true, true, 0};
Btn btnDown {PIN_BTN_DOWN, true, true, 0};
Btn btnOk   {PIN_BTN_OK,   true, true, 0};

// Debounced falling-edge (active LOW) reader
bool readButtonEdge(Btn &b) {
  const uint32_t now = millis();
  bool raw = digitalRead(b.pin);
  if (raw != b.lastStable && (now - b.lastChangeMs) > 25) {
    b.lastStable   = raw;
    b.lastChangeMs = now;
    // edge detection: HIGH -> LOW = press
    if (b.state == HIGH && raw == LOW) {
      b.state = raw;
      return true;
    }
    b.state = raw;
  }
  return false;
}

// ---------- Motor helpers ----------
static inline void motorStop() {
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
}
static inline void motorDrive(int pwm, bool dirPositive) {
  pwm = constrain(pwm, 0, MAX_PWM);
  if (!motorEnabled || pwm == 0) { motorStop(); return; }
  bool dir = dirPositive ^ invertMotor;
  if (dir) {
    analogWrite(PIN_IN1, pwm);
    digitalWrite(PIN_IN2, LOW);
  } else {
    digitalWrite(PIN_IN1, LOW);
    analogWrite(PIN_IN2, pwm);
  }
}

// ---------- UI helpers ----------
void drawMainUI(int pos) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(F("TX CC#")); display.print(CC_NUM_TX); display.print(F(" Ch2  "));
  display.print(F("RX CC#")); display.print(CC_NUM_RX); display.print(F(" Ch1"));

  display.setCursor(0, 12);
  display.print(F("Mode: "));
  display.println(sourceMode == SourceMode::FEEDBACK ? F("FEEDBACK") : F("LOCAL"));

  display.setCursor(0, 24);
  display.print(F("Target: ")); display.println(targetADC);
  display.setCursor(0, 34);
  display.print(F("Pos:    ")); display.println(pos);

  display.drawRect(0, 52, 120, 10, SSD1306_WHITE);
  int barPos = map(pos, 0, ADC_MAX, 0, 120);
  display.fillRect(0, 52, barPos, 10, SSD1306_WHITE);

  display.display();
}
void drawMenu() {
  display.setTextSize(1);
  display.fillRect(0, 44, 128, 20, SSD1306_BLACK);
  display.setCursor(0, 44);
  switch (currentItem) {
    case MenuItem::SOURCE:
      display.print(F("Source: "));
      display.println(sourceMode == SourceMode::FEEDBACK ? F("FEEDBACK") : F("LOCAL"));
      break;
    case MenuItem::LOCAL_VALUE:
      display.print(F("Local Val: "));
      display.println((int)localValue);
      break;
    case MenuItem::KP:
      display.print(F("Kp: "));
      display.println(kp_x10 / 10.0f, 1);
      break;
    case MenuItem::KI:
      display.print(F("Ki: "));
      display.println(ki_x1000 / 1000.0f, 3);
      break;
    case MenuItem::DEADBAND:
      display.print(F("Deadband: "));
      display.println(deadband);
      break;
    case MenuItem::MIN_PWM:
      display.print(F("MinPWM: "));
      display.println(minPWM);
      break;
    case MenuItem::MOTOR_ENABLE:
      display.print(F("Motor: "));
      display.println(motorEnabled ? F("EN") : F("DIS"));
      break;
    case MenuItem::INVERT_MOTOR:
      display.print(F("Invert: "));
      display.println(invertMotor ? F("ON") : F("OFF"));
      break;
    default: break;
  }
  display.display();
}
void nextMenuItem() {
  uint8_t i = (uint8_t)currentItem;
  i = (i + 1) % (uint8_t)MenuItem::_COUNT;
  currentItem = (MenuItem)i;
}
void adjustCurrentItem(int delta) {
  switch (currentItem) {
    case MenuItem::SOURCE:
      if (delta != 0)
        sourceMode = (sourceMode == SourceMode::FEEDBACK) ? SourceMode::LOCAL : SourceMode::FEEDBACK;
      break;
    case MenuItem::LOCAL_VALUE:
      localValue = constrain((int)localValue + delta, 0, 127);
      break;
    case MenuItem::KP:
      kp_x10 = constrain(kp_x10 + delta, 1, 300);          // 0.1 .. 30.0
      break;
    case MenuItem::KI:
      ki_x1000 = constrain(ki_x1000 + delta * 5, 0, 1000); // step 0.005
      break;
    case MenuItem::DEADBAND:
      deadband = constrain(deadband + delta, 0, 50);
      break;
    case MenuItem::MIN_PWM:
      minPWM = constrain(minPWM + delta * 5, 0, 255);
      break;
    case MenuItem::MOTOR_ENABLE:
      if (delta != 0) motorEnabled = !motorEnabled;
      break;
    case MenuItem::INVERT_MOTOR:
      if (delta != 0) invertMotor = !invertMotor;
      break;
    default: break;
  }
}

// --------------- Setup ---------------
void setup() {
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.display();

  pinMode(PIN_IN1, OUTPUT);
  pinMode(PIN_IN2, OUTPUT);
  motorStop();

  pinMode(PIN_BTN_UP,   INPUT_PULLUP);
  pinMode(PIN_BTN_DOWN, INPUT_PULLUP);
  pinMode(PIN_BTN_OK,   INPUT_PULLUP);

  analogReadResolution(12);
  Control_Surface.begin();

  targetADC = analogRead(PIN_FADER_ADC); // avoid jump on boot
}

// --------------- Loop ----------------
void loop() {
  Control_Surface.loop();  // handles TX from potOut and RX into ccIn

  // Buttons / menu
  bool up = readButtonEdge(btnUp);
  bool dn = readButtonEdge(btnDown);
  bool ok = readButtonEdge(btnOk);
  if (ok) nextMenuItem();
  if (up) adjustCurrentItem(+1);
  if (dn) adjustCurrentItem(-1);

  // Target source
  if (sourceMode == SourceMode::FEEDBACK) {
    if (ccIn.getDirty()) {
      uint8_t v = ccIn.getValue();            // 0..127
      targetADC = map(v, 0, 127, 0, ADC_MAX); // -> 0..4095
      ccIn.clearDirty();
    }
  } else {
    targetADC = map((int)localValue, 0, 127, 0, ADC_MAX);
  }

  // Position & PI
  int pos = analogRead(PIN_FADER_ADC);
  int err = targetADC - pos;

  if (!motorEnabled) {
    motorStop();
    integral = 0.0f;
  } else if (abs(err) <= deadband) {
    motorStop();
    integral *= 0.9f;
  } else {
    integral += err;
    if (integral >  8000.0f) integral =  8000.0f;
    if (integral < -8000.0f) integral = -8000.0f;

    float Kp = kp_x10 / 10.0f;
    float Ki = ki_x1000 / 1000.0f;
    float u  = Kp * err + Ki * integral;

    int pwmVal = constrain(abs((int)u), 0, MAX_PWM);
    if (pwmVal > 0 && pwmVal < minPWM) pwmVal = minPWM;
    motorDrive(pwmVal, (u > 0));
  }

  // OLED
  drawMainUI(pos);
  drawMenu();
}