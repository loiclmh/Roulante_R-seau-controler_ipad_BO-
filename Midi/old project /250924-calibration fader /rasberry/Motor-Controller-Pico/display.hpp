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
    Wire.begin(); // SDA=GP4, SCL=GP5 (RP2040 par défaut selon ton montage)
    display.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR);
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.display();
  }

  void showSplash(const char* text) {
    display.clearDisplay();
    display.setCursor(0, 0);
    display.setTextSize(1);
    display.println(F("Pico Fader Test"));
    display.println(text);
    display.display();
  }

  // Affiche barre fader + valeur moteur
  void showFaderAndMotor(uint16_t y_smooth_10b, uint16_t y_raw_10b, int16_t u_cmd) {
    display.clearDisplay();

    // Ligne 1 : titres + valeurs
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Raw: ")); display.print(y_raw_10b);
    display.setCursor(72, 0);
    display.print(F("EMA: ")); display.print(y_smooth_10b);

    // Barre 0..1023 => 0..120 px
    int bar = map((int)y_smooth_10b, 0, 1023, 0, 120);
    display.drawRect(4, 16, 120, 12, SSD1306_WHITE);
    display.fillRect(4, 16, bar, 12, SSD1306_WHITE);

    // Gradations
    display.drawLine(4, 28, 4, 14, SSD1306_WHITE);        // 0%
    display.drawLine(64, 28, 64, 14, SSD1306_WHITE);      // 50%
    display.drawLine(124, 28, 124, 14, SSD1306_WHITE);    // 100%

    display.setCursor(4, 32);
    display.print(F("u: ")); display.print(u_cmd);
    display.print(F("  "));
    if (u_cmd > 12)      display.print(F(">>"));
    else if (u_cmd < -12) display.print(F("<<"));
    else                 display.print(F("--"));

    display.display();
  }
}
