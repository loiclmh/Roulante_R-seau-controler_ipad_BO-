// Fader_Pico_OLED_MIDIUSB_Motor.ino
// Raspberry Pi Pico (RP2040) — Fader motorisé + OLED + USB-MIDI (TinyUSB)
// - Lit le fader (ADC), envoie CC#7 ch.1
// - Reçoit CC#7 → suit la consigne avec PID via DRV8871 (IN1/IN2 PWM)
// - Affiche sur OLED : ADC, CC Out, CC In, PWM Effort
// IDE: Board = Raspberry Pi Pico | Tools → USB Stack = TinyUSB
// Libs: Adafruit_TinyUSB, MIDI (FortySevenEffects), Adafruit_GFX, Adafruit_SSD1306

#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MIDI usb_midi;
#include <MIDI.h>
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIUSB);

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// --------- PINS (Pico) ----------
static const uint8_t PIN_FADER_WIPER = 26; // A0 / GP26 / ADC0
static const uint8_t PIN_MOTOR_IN1   = 2;  // -> DRV8871 IN1 (PWM A)
static const uint8_t PIN_MOTOR_IN2   = 3;  // -> DRV8871 IN2 (PWM B)
static const uint8_t PIN_TOUCH       = 22; // optionnel (LOW si touché), sinon laisser en l'air

// I2C OLED
static const uint8_t I2C_SDA = 4;          // GP4
static const uint8_t I2C_SCL = 5;          // GP5
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --------- MIDI ---------
static const uint8_t MIDI_CH = 1;
static const uint8_t MIDI_CC = 7; // Volume

// --------- ADC / Filtrage / Calib ---------
static const int      ADC_MAX_VAL    = 4095; // 12 bits
static const uint8_t  SMOOTH_ALPHA   = 12;   // 1..16 (↑ = plus lisse)
static const uint16_t SEND_MS        = 10;   // rate limit envoi CC
static const uint8_t  HYSTERESIS_CC  = 1;    // delta min CC
static const uint32_t CALIB_MS       = 5000; // bouge le fader ~5s au boot
static const int      CALIB_MIN_SPAN = 800;  // span mini pour valider
static const int      MARGIN_COUNTS  = 16;   // marge anti-butées
bool invertCC = false; // inverser le sens logiciel si besoin

// --------- PID / Moteur ---------
static const uint32_t LOOP_HZ  = 800; // boucle PID
static const int      DEADBAND = 12;
static const int      PWM_MAX  = 255;
static const int      MAX_PWM  = 180;   // 150 pour 5V / 180–210 pour 12V costaud
static const float    KP       = 0.20f;
static const float    KI       = 0.0008f;
static const float    KD       = 0.10f;
static const int      I_CLAMP  = 3000;
static const int      MANUAL_TH= 12;    // seuil mouvement main (ADC)

// --------- État ---------
uint32_t ema = 0;         // filtre EMA <<8
int lastCC = -1;
uint32_t lastSend = 0;

bool calibrated = false;
uint32_t t0 = 0;
int adcMinSeen = ADC_MAX_VAL;
int adcMaxSeen = 0;

volatile int targetADC = -1; // consigne en ADC (map du CC reçu)
int lastADC = 0;
int pwmEffort = 0;

int lastErr = 0;
int32_t integrator = 0;
uint32_t nextLoopMicros = 0;

// ---------- Helpers ----------
static inline int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }
static inline int readADC(){ return analogRead(PIN_FADER_WIPER); }

static inline uint8_t mapADCtoCC(int adc, int aMin, int aMax){
  adc = clampi(adc, aMin, aMax);
  int cc = map(adc, aMin, aMax, 0, 127);
  cc = clampi(cc, 0, 127);
  if (invertCC) cc = 127 - cc;
  return (uint8_t)cc;
}

inline void driveMotor(int effort){
  effort = clampi(effort, -MAX_PWM, MAX_PWM);
  if (effort > 0){ analogWrite(PIN_MOTOR_IN1, effort); analogWrite(PIN_MOTOR_IN2, 0); }
  else if (effort < 0){ analogWrite(PIN_MOTOR_IN1, 0); analogWrite(PIN_MOTOR_IN2, -effort); }
  else { analogWrite(PIN_MOTOR_IN1, 0); analogWrite(PIN_MOTOR_IN2, 0); }
}

// MIDI IN (CC) → set targetADC
void onCC(byte ch, byte num, byte val){
  if (ch != MIDI_CH || num != MIDI_CC) return;
  const int margin = MARGIN_COUNTS;
  targetADC = map(val, 0, 127, margin, ADC_MAX_VAL - margin);
}

// OLED UI
void drawUI(int adc, uint8_t ccOut, int ccIn, int effort){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);

  display.print("ADC  : "); display.println(adc);
  display.print("CCOut: "); display.println(ccOut);
  display.print("CCIn : ");
  if (ccIn >= 0) display.println(ccIn); else display.println("-");
  display.print("PWM  : "); display.println(effort);

  int w = map(ccOut, 0, 127, 0, SCREEN_WIDTH - 4);
  display.drawRect(0, 48, SCREEN_WIDTH-1, 14, SSD1306_WHITE);
  display.fillRect(2, 50, w, 10, SSD1306_WHITE);
  display.display();
}

// --------- Setup ---------
void setup(){
  // Moteur
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_TOUCH, INPUT_PULLUP); // si non câblé, restera HIGH

  analogWriteRange(PWM_MAX);
  analogWriteFreq(25000);      // 25 kHz (silencieux)
  analogReadResolution(12);    // 0..4095

  // OLED
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Pico Motor Fader");
  display.println("MIDIUSB + OLED");
  display.display();

  // MIDI
  MIDIUSB.begin(MIDI_CH);
  MIDIUSB.setHandleControlChange(onCC);

  // Filtre / Calib
  int a0 = readADC();
  ema = (uint32_t)a0 << 8;
  t0 = millis();
  adcMinSeen = ADC_MAX_VAL;
  adcMaxSeen = 0;

  lastADC = a0;
  nextLoopMicros = micros();
}

// --------- Loop ---------
void loop(){
  // MIDI in
  MIDIUSB.read();

  // Lecture + lissage
  int raw = readADC();
  ema = ema + (((uint32_t)raw << 8) - ema) / SMOOTH_ALPHA;
  int smooth = (int)(ema >> 8);

  // Auto-calibration
  if (!calibrated){
    if (smooth < adcMinSeen) adcMinSeen = smooth;
    if (smooth > adcMaxSeen) adcMaxSeen = smooth;

    if (millis() - t0 >= CALIB_MS){
      if ((adcMaxSeen - adcMinSeen) < CALIB_MIN_SPAN){
        adcMinSeen = 64;
        adcMaxSeen = ADC_MAX_VAL - 64;
      }
      adcMinSeen = clampi(adcMinSeen + MARGIN_COUNTS, 0, ADC_MAX_VAL - 2*MARGIN_COUNTS);
      adcMaxSeen = clampi(adcMaxSeen - MARGIN_COUNTS, adcMinSeen + 10, ADC_MAX_VAL);
      calibrated = true;
    }
  }

  int aMin = calibrated ? adcMinSeen : 0;
  int aMax = calibrated ? adcMaxSeen : ADC_MAX_VAL;

  // Envoi CC (mouvement manuel + rate-limit)
  uint8_t ccOut = mapADCtoCC(smooth, aMin, aMax);
  uint32_t now = millis();
  if (now - lastSend >= SEND_MS){
    if (lastCC < 0 || abs((int)ccOut - lastCC) >= HYSTERESIS_CC){
      MIDIUSB.sendControlChange(MIDI_CC, ccOut, MIDI_CH);
      lastCC = ccOut;
      lastSend = now;
    }
  }

  // PID moteur à cadence fixe
  uint32_t tnow = micros();
  if ((int32_t)(tnow - nextLoopMicros) >= 0){
    nextLoopMicros += 1000000UL / LOOP_HZ;

    bool touched = (digitalRead(PIN_TOUCH) == LOW);
    int effort = 0;

    // Détection mouvement main -> n’envoie pas d’effort si touché
    if (abs(smooth - lastADC) > MANUAL_TH && touched){
      targetADC = -1; // on relâche la consigne si on touche (pickup)
    }

    if (targetADC >= 0 && !touched){
      int err = targetADC - smooth;

      integrator = clampi(integrator + err, -I_CLAMP, I_CLAMP);
      int d = err - lastErr;
      float out = KP * (float)err + KI * (float)integrator + KD * (float)d;

      if (abs(err) <= DEADBAND){
        out = 0;
        integrator = (int32_t)((float)integrator * 0.95f);
      }

      int maxEff = map(clampi(abs(err), 0, 800), 0, 800, 40, MAX_PWM);
      effort = clampi((int)out, -maxEff, maxEff);
      lastErr = err;
    } else {
      // relâchement doux
      integrator = (int32_t)((float)integrator * 0.98f);
      lastErr = 0;
      effort = 0;
    }

    driveMotor(effort);
    pwmEffort = effort;
    lastADC = smooth;
  }

  // OLED (50 ms)
  static uint32_t nextDraw = 0;
  if (now >= nextDraw){
    int ccIn = -1;
    if (targetADC >= 0){
      int tMin = MARGIN_COUNTS, tMax = ADC_MAX_VAL - MARGIN_COUNTS;
      int cc = map(clampi(targetADC, tMin, tMax), tMin, tMax, 0, 127);
      ccIn = clampi(cc, 0, 127);
    }
    drawUI(smooth, ccOut, ccIn, pwmEffort);
    nextDraw = now + 50;
  }
}

