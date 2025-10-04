#include <Arduino.h>
#include "display.h"

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>



static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setupOLED() {
    //set up écran oled 
  Wire.setSDA(4);                 // SDA
  Wire.setSCL(5);                 // SCL
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("[OLED] Échec init SSD1306 @0x3C — vérifie SDA=4 SCL=5 et l'alim.");
    return;
  }
  oled.clearDisplay();
  oled.setTextSize(3);
  oled.setTextColor(SSD1306_WHITE);
  oled.print("pico ok");
  oled.setCursor(0, 30);
  oled.setTextSize(1);
  oled.print("version info fader OK");
  oled.display();
}



void drawOLED(int value) {
  // Affiche la valeur du fader 0 passée en paramètre
  oled.clearDisplay();

  oled.setCursor(0, 0);
  oled.print("ADC: ");
  oled.println(value);

  int barLength = map(value, 0, 4095, 0, SCREEN_WIDTH);
  oled.fillRect(0, 20, barLength, 10, SSD1306_WHITE);

  oled.display();
}

