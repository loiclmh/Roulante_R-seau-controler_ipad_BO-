// Fader_Pico_OLED_ControlSurface.ino
// RP2040 (Pico) + Control Surface + OLED SSD1306 (TX-only)
// Envoie CC#7 (ch.1) via USB MIDI avec Control Surface
// Affiche sur OLED : ADC (0..4095) et CC envoyé (0..127)

#include <Control_Surface.h>        // tttapa/Control Surface
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- MIDI USB (Control Surface) ----------
USBMIDI_Interface midi;  // périphérique USB-MIDI

// ---------- Élément MIDI : fader -> CC#7 ch.1 ----------
CCPotentiometer fader {
  A0,                                         // Pin wiper (Pico A0 = GP26)
  {MIDI_CC::Channel_Volume, Channel_1}       // CC#7 sur canal 1
};

// ---------- OLED 128x64 I2C ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_ADDR 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// ---------- PINS I2C PICO ----------
static const uint8_t I2C_SDA = 4;   // GP4
static const uint8_t I2C_SCL = 5;   // GP5

// ---------- Helpers d'affichage ----------
static inline int clampi(int v, int lo, int hi){ return v<lo?lo:(v>hi?hi:v); }

void drawUI(int adc, int cc) {
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.print("ADC  : "); display.println(adc);
  display.print("CC#7 : "); display.println(cc);

  // Barre 0..127
  int w = map(cc, 0, 127, 0, SCREEN_WIDTH - 4);
  display.drawRect(0, 48, SCREEN_WIDTH - 1, 14, SSD1306_WHITE);
  display.fillRect(2, 50, w, 10, SSD1306_WHITE);

  display.display();
}

void setup() {
  // ADC 12 bits sur RP2040
  analogReadResolution(12);

  // I2C / OLED
  Wire.setSDA(I2C_SDA);
  Wire.setSCL(I2C_SCL);
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("Control Surface");
  display.println("Pico Fader + OLED");
  display.display();

  // Control Surface
  Control_Surface.begin();   // démarre USB MIDI et enregistre les éléments
}

void loop() {
  // Laisse Control Surface gérer lecture fader + envoi CC
  Control_Surface.loop();

  // Pour l'affichage, on lit l'ADC et on mappe à 0..127 (indicatif)
  int adc = analogRead(A0);                       // 0..4095
  int cc  = clampi(map(adc, 0, 4095, 0, 127), 0, 127);

  // Affichage (toutes ~50 ms)
  static uint32_t nextDraw = 0;
  uint32_t now = millis();
  if (now >= nextDraw) {
    drawUI(adc, cc);
    nextDraw = now + 50;
  }
}
