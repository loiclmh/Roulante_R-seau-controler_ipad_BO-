#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "display.h"
#include "pins.h"

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void display_begin() {
  Wire.setSDA(PIN_I2C_SDA);
  Wire.setSCL(PIN_I2C_SCL);
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_1) &&
      !oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_2)) {
    while (1) { delay(10); }
  }
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.println("OLED READY");
  oled.display();
}

void display_status(const String& l1, const String& l2, const String& l3) {
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.println(l1);
  oled.setCursor(0, 10); oled.println(l2);
  oled.setCursor(0, 20); oled.println(l3);
  oled.display();
}

void display_fader(int raw, int minv, int maxv, const String& note, int out8) {
  int span = maxv - minv; if (span < 1) span = 1;
  int clamped = raw; if (clamped < minv) clamped = minv; if (clamped > maxv) clamped = maxv;
  int bar_w = (clamped - minv) * (OLED_WIDTH - 2) / span;

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("Fader "); oled.println(note);
  oled.setCursor(0, 10); oled.print("RAW=");  oled.print(raw);
  oled.setCursor(0, 20); oled.print("OUT8="); oled.print(out8);

  oled.drawRect(0, 32, OLED_WIDTH, 12, SSD1306_WHITE);
  if (bar_w > 0) oled.fillRect(1, 33, bar_w, 10, SSD1306_WHITE);
  oled.display();
}