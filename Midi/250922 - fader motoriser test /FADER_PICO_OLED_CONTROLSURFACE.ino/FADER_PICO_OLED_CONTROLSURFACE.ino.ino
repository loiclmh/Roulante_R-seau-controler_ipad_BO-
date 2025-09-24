// Fader_Pico_OLED_ControlSurface_Motor.ino
// RP2040 (Pico) — Control Surface (USB MIDI) + OLED SSD1306 + DRV8871 (motor PID)

#include <Control_Surface.h>             // tttapa/Control Surface
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ───── MIDI (Control Surface) ─────
USBMIDI_Interface midi;

// Fader → envoi CC#7 ch.1
CCPotentiometer faderOut {
  A0, {MIDI_CC::Channel_Volume, Channel_1}
};

// Réception CC#7 ch.1 (consigne)
CCValue faderIn { {MIDI_CC::Channel_Volume, Channel_1} };

// ───── OLED ─────
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static const uint8_t I2C_SDA = 4;  // GP4
static const uint8_t I2C_SCL = 5;  // GP5

// ───── Pins moteur (DRV8871) ─────
static const uint8_t PIN_MOTOR_IN1 = 2;   // IN1 (PWM)
static const uint8_t PIN_MOTOR_IN2 = 3;   // IN2 (PWM)
static const uint8_t PIN_TOUCH     = 22;  // optionnel: LOW si touché (sinon laisser en l'air)

// ───── ADC / Calib / Mapping ─────
static const int      ADC_MAX_VAL    = 4095; // 12 bits
static const uint8_t  SMOOTH_ALPHA   = 12;   // 1..16 (↑ plus lisse)
static const uint32_t CALIB_MS       = 5000; // bouger le fader ~5 s au boot
static const int      CALIB_MIN_SPAN = 800;
static const int      MARGIN_COUNTS  = 16;
bool invertCC = false;

// ───── Envoi CC (anti-spam) ─────
static const uint16_t SEND_MS        = 10;
static const uint8_t  HYSTERESIS_CC  = 1;

// ───── PID moteur ─────
static const uint32_t LOOP_HZ  = 800;
static const int      DEADBAND = 12;
static const int      PWM_MAX  = 255;
static const int      MAX_PWM  = 200;   // 150 pour 5 V / 200–210 pour 12 V costaud
static const float    KP       = 0.20f;
static const float    KI       = 0.0008f;
static const float    KD       = 0.10f;
static const int      I_CLAMP  = 3000;
static const int      MANUAL_TH= 12;

// ───── État ─────
uint32_t ema = 0; // filtre EMA <<8
bool calibrated = false;
uint32_t t0 = 0;
int adcMinSeen = ADC_MAX_VAL, adcMaxSeen = 0;

int lastADC = 0, lastErr = 0, pwmEffort = 0;
int32_t integrator = 0;
uint32_t nextLoopMicros = 0;

int lastCCSent = -1;
uint32_t lastSend = 0;

static inline int clampi(int v,int lo,int hi){ return v<lo?lo:(v>hi?hi:v); }
static inline int readADC(){ return analogRead(A0); }

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

void drawUI(int adc, int ccOut, int ccIn, int effort){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);

  display.print("ADC  : "); display.println(adc);
  display.print("CCOut: "); display.println(ccOut);
  display.print("CCIn : "); display.println(ccIn >= 0 ? ccIn : -1);
  display.print("PWM  : "); display.println(effort);

  int w = map(clampi(ccOut,0,127), 0, 127, 0, SCREEN_WIDTH-4);
  display.drawRect(0, 48, SCREEN_WIDTH-1, 14, SSD1306_WHITE);
  display.fillRect(2, 50, w, 10, SSD1306_WHITE);
  display.display();
}

void setup(){
  // ADC
  analogReadResolution(12);

  // I2C / OLED
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.println("Control Surface + PID");
  display.println("Pico Motor Fader");
  display.display();

  // Moteur
  pinMode(PIN_MOTOR_IN1, OUTPUT);
  pinMode(PIN_MOTOR_IN2, OUTPUT);
  pinMode(PIN_TOUCH, INPUT_PULLUP);
  analogWriteRange(PWM_MAX);
  analogWriteFreq(25000); // 25 kHz

  // Control Surface (USB MIDI, faderOut, faderIn)
  Control_Surface.begin();

  // Init filtre/calib
  int a0 = readADC();
  ema = (uint32_t)a0 << 8;
  t0 = millis();
  adcMinSeen = ADC_MAX_VAL;
  adcMaxSeen = 0;

  lastADC = a0;
  nextLoopMicros = micros();
}

void loop(){
  // Laisse Control Surface gérer I/O MIDI
  Control_Surface.loop();

  // Lecture + lissage
  int raw = readADC();
  ema = ema + (((uint32_t)raw<<8) - ema) / SMOOTH_ALPHA;
  int smooth = (int)(ema >> 8);

  // Auto-calibration au boot
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

  // Envoi CC (anti-spam indicatif) — Control Surface envoie déjà,
  // mais on garde un affichage cohérent basé sur l'ADC mappé.
  int ccOut = mapADCtoCC(smooth, aMin, aMax);
  uint32_t now = millis();
  if (now - lastSend >= SEND_MS){
    if (lastCCSent < 0 || abs(ccOut - lastCCSent) >= HYSTERESIS_CC){
      // (Optionnel) on peut forcer un renvoi :
      // midi.sendCC({MIDI_CC::Channel_Volume, Channel_1}, ccOut);
      lastCCSent = ccOut;
      lastSend = now;
    }
  }

  // Consigne depuis le réseau/USB via Control Surface
  // Valeur 0..127, on la mappe en cible ADC
  int ccIn = faderIn.getValue(); // retourne le dernier CC reçu (0..127)
  // Si tu veux ignorer tant qu'aucun CC reçu, tu peux gérer un flag "vu au moins 1 CC".
  int targetADC = map(ccIn, 0, 127, MARGIN_COUNTS, ADC_MAX_VAL - MARGIN_COUNTS);

  // Boucle PID (cadence fixe)
  uint32_t tnow = micros();
  if ((int32_t)(tnow - nextLoopMicros) >= 0){
    nextLoopMicros += 1000000UL / LOOP_HZ;

    bool touched = (digitalRead(PIN_TOUCH) == LOW);
    int effort = 0;

    // Mouvement manuel détecté → on laisse la main si touché
    if (abs(smooth - lastADC) > MANUAL_TH && touched) {
      // on "gèle" l'effort tant que touché
      effort = 0;
      integrator = (int32_t)((float)integrator * 0.98f);
      lastErr = 0;
    } else {
      // PID vers la consigne reçue (ccIn)
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
    }

    driveMotor(effort);
    pwmEffort = effort;
    lastADC = smooth;
  }

  // Affichage OLED (toutes ~50 ms)
  static uint32_t nextDraw = 0;
  if (now >= nextDraw){
    drawUI(smooth, ccOut, ccIn, pwmEffort);
    nextDraw = now + 50;
  }
}
