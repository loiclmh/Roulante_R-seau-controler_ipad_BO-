#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#if __has_include("pins.h")
  #include "pins.h"
  #ifndef OLED_I2C_ADDR
    #define OLED_I2C_ADDR 0x3C
  #endif
#else
  #define OLED_I2C_ADDR 0x3C
#endif

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace Display {

  void begin() {
    Wire.begin();  // Pico : SDA=GP4, SCL=GP5 (ton montage)
    if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
      // Ne pas bloquer : log si port série dispo
      digitalWrite(PIN_DEBUG_LED, HIGH);
      delay(200);
      digitalWrite(PIN_DEBUG_LED, LOW);
      Serial.begin(115200);
      Serial.println(F("OLED non detecte !"));
      return;
    }
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);
    display.println(F("OLED Ready!"));
    display.display();
    delay(300);
  }

  void showMessage(const char* text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println(text);
    display.display();
  }

  void showCounter(int value) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println(F("Compteur:"));
    display.setCursor(0, 16);
    display.setTextSize(2);
    display.println(value);
    display.display();
  }
}
