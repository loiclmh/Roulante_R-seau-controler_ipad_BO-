#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int PIN_FADER = A0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);       // 🔥 Force 12 bits
  Wire.setSDA(4);                 // SDA de ton montage
  Wire.setSCL(5);                 // SCL de ton montage
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);                     // Bloque si écran non détecté
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
}

void loop() {
  int raw = analogRead(PIN_FADER); // 0..4095
  Serial.println(raw);

  oled.clearDisplay();

  // Affiche la valeur brute
  oled.setCursor(0, 0);
  oled.print("ADC: ");
  oled.println(raw);

  // Affiche une barre horizontale selon la valeur
  int barLength = map(raw, 0, 4095, 0, SCREEN_WIDTH);
  oled.fillRect(0, 20, barLength, 10, SSD1306_WHITE);

  oled.display();

  delay(20); // rafraîchit ~50 Hz
}