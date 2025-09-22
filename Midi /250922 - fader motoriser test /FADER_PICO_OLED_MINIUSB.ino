// Pico RP2040 — Fader → USB-MIDI CC + OLED SSD1306 (TX-only)
// Tools -> Board: Raspberry Pi Pico | USB Stack: TinyUSB

#include <Adafruit_TinyUSB.h>
Adafruit_USBD_MIDI usb_midi;
#include <MIDI.h>
MIDI_CREATE_INSTANCE(Adafruit_USBD_MIDI, usb_midi, MIDIUSB);

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- Pins ----------
static const uint8_t PIN_FADER_WIPER = 26; // A0 / GP26 / ADC0
static const uint8_t I2C_SDA = 4;          // GP4
static const uint8_t I2C_SCL = 5;          // GP5

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- MIDI ----------
static const uint8_t MIDI_CH = 1;
static const uint8_t MIDI_CC = 7;

// ---------- ADC & filtre ----------
static const int      ADC_MAX_VAL   = 4095; // 12 bits
static const uint8_t  SMOOTH_ALPHA  = 12;   // 1..16
static const uint16_t SEND_MS       = 10;   // rate limit
static const uint8_t  HYSTERESIS_CC = 1;    // delta min CC
static const uint32_t CALIB_MS      = 5000; // bouger fader 5s au boot
static const int      CALIB_MIN_SPAN= 800;
static const int      MARGIN_COUNTS = 16;

bool invertCC = false;

// ---------- État ----------
uint32_t ema = 0;     // filtre EMA <<8
int lastCC = -1;
uint32_t lastSend = 0;

bool calibrated = false;
uint32_t t0 = 0;
int adcMinSeen = ADC_MAX_VAL;
int adcMaxSeen = 0;

volatile int targetADC = -1; // pour affichage (si un jour on lit le MIDI In)

// ---------- Helpers ----------
static inline int clampi(int v,int lo,int hi){return v<lo?lo:(v>hi?hi:v);}
static inline int readADC(){ return analogRead(PIN_FADER_WIPER); }

static inline uint8_t mapADCtoCC(int adc,int aMin,int aMax){
  adc = clampi(adc, aMin, aMax);
  int cc = map(adc, aMin, aMax, 0, 127);
  cc = clampi(cc, 0, 127);
  if (invertCC) cc = 127 - cc;
  return (uint8_t)cc;
}

void drawUI(int adc, uint8_t ccOut, int ccIn){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);

  display.print("ADC  : "); display.println(adc);
  display.print("CC Out: "); display.println(ccOut);
  display.print("CC In : ");
  if (ccIn >= 0) display.println(ccIn); else display.println("-");

  // Barre 0..127
  int w = map(ccOut, 0, 127, 0, SCREEN_WIDTH - 4);
  display.drawRect(0, 48, SCREEN_WIDTH-1, 14, SSD1306_WHITE);
  display.fillRect(2, 50, w, 10, SSD1306_WHITE);

  display.display();
}

// ---------- Setup ----------
void setup() {
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
  display.println("Pico Fader + OLED");
  display.println("TX-only / Calib 5s");
  display.display();

  // MIDI USB
  MIDIUSB.begin(MIDI_CH);

  // Filtre & calib
  int a0 = readADC();
  ema = (uint32_t)a0 << 8;
  t0 = millis();
  adcMinSeen = ADC_MAX_VAL;
  adcMaxSeen = 0;
}

// ---------- Loop ----------
void loop() {
  // Lecture + lissage
  int raw = readADC();
  ema = ema + (((uint32_t)raw<<8) - ema) / SMOOTH_ALPHA;
  int smooth = (int)(ema >> 8);

  // Auto-calibration (5 s)
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

  // CC out (rate-limit + hystérésis)
  uint8_t ccOut = mapADCtoCC(smooth, aMin, aMax);
  uint32_t now = millis();
  if (now - lastSend >= SEND_MS){
    if (lastCC < 0 || abs((int)ccOut - lastCC) >= HYSTERESIS_CC){
      MIDIUSB.sendControlChange(MIDI_CC, ccOut, MIDI_CH);
      lastCC = ccOut;
      lastSend = now;
    }
  }

  // Affichage OLED (toutes ~50 ms)
  static uint32_t nextDraw = 0;
  if (now >= nextDraw){
    int ccIn = -1; // TX-only (pas de lecture CC In ici)
    drawUI(smooth, ccOut, ccIn);
    nextDraw = now + 50;
  }
}
