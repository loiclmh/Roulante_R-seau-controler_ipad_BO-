#include <Arduino.h>

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
    Wire.begin();
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

  void showFaderAndMotor(uint16_t y_smooth_10b, uint16_t y_raw_10b, int16_t u_cmd, bool touched) {
    display.clearDisplay();

    // Header: raw & EMA
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print(F("Raw: ")); display.print(y_raw_10b);
    display.setCursor(72, 0);
    display.print(F("EMA: ")); display.print(y_smooth_10b);

    // TOUCH badge
    if (touched) {
      const int bx = 92, by = 0, bw = 36, bh = 10;
      display.fillRect(bx, by, bw, bh, SSD1306_WHITE);
      display.setTextColor(SSD1306_BLACK);
      display.setCursor(bx + 3, by + 1);
      display.print(F("TOUCH"));
      display.setTextColor(SSD1306_WHITE);
    }

    // Main bar for current fader position (EMA)
    const int bar_x = 4, bar_y = 16, bar_w = 120, bar_h = 12;
    int bar = map((int)y_smooth_10b, 0, 1023, 0, bar_w);
    display.drawRect(bar_x, bar_y, bar_w, bar_h, SSD1306_WHITE);
    display.fillRect(bar_x, bar_y, bar, bar_h, SSD1306_WHITE);

    // Mid/end ticks
    display.drawLine(bar_x,           bar_y + bar_h, bar_x,           bar_y - 2, SSD1306_WHITE);
    display.drawLine(bar_x + bar_w/2, bar_y + bar_h, bar_x + bar_w/2, bar_y - 2, SSD1306_WHITE);
    display.drawLine(bar_x + bar_w,   bar_y + bar_h, bar_x + bar_w,   bar_y - 2, SSD1306_WHITE);

    // If a reference is set through the extended overload, a thin marker will be drawn.
    // (Handled in the extended overload below.)

    // Motor command and direction
    display.setCursor(4, 32);
    display.print(F("u: ")); display.print(u_cmd);
    display.print(F("  "));
    if (u_cmd > 12)      display.print(F(">>"));
    else if (u_cmd < -12) display.print(F("<<"));
    else                 display.print(F("--"));

    // Footer reserved for MIDI badge from the extended overload.

    display.display();
  }

  void showFaderAndMotor(uint16_t y_smooth_10b, uint16_t y_raw_10b, int16_t u_cmd, bool touched, int ref_10b, bool midi_rx) {
    // Draw base UI
    showFaderAndMotor(y_smooth_10b, y_raw_10b, u_cmd, touched);

    // If a reference is provided (0..1023), draw a thin vertical marker on top of the bar
    if (ref_10b >= 0 && ref_10b <= 1023) {
      const int bar_x = 4, bar_y = 16, bar_w = 120, bar_h = 12;
      int x = map(ref_10b, 0, 1023, bar_x, bar_x + bar_w);
      // Draw a thin marker line
      display.drawLine(x, bar_y - 3, x, bar_y + bar_h + 3, SSD1306_WHITE);
      display.display();
    }

    // MIDI RX badge (bottom-right)
    if (midi_rx) {
      const int bx = 96, by = 48, bw = 28, bh = 12;
      display.drawRect(bx, by, bw, bh, SSD1306_WHITE);
      display.setCursor(bx + 3, by + 2);
      display.setTextSize(1);
      display.print(F("MIDI"));
      display.display();
    }
  }

  inline void showFaderAndMotor(uint16_t y_smooth_10b, uint16_t y_raw_10b, int16_t u_cmd) {
    showFaderAndMotor(y_smooth_10b, y_raw_10b, u_cmd, false);
  }

  inline void showFaderAndMotor(uint16_t y_smooth_10b, uint16_t y_raw_10b, int16_t u_cmd, bool touched, bool midi_rx) {
    showFaderAndMotor(y_smooth_10b, y_raw_10b, u_cmd, touched, -1, midi_rx);
  }
}
