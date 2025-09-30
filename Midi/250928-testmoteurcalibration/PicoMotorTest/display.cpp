#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "pins.h"

static Adafruit_SSD1306 oled(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

void display_begin() {
  Wire.setSDA(PIN_I2C_SDA);
  Wire.setSCL(PIN_I2C_SCL);
  Wire.begin();
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_1)) {
    if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_2)) {
      while (1) {}
    }
  }
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.display();
}

static void printLine(int y, const String& s) {
  oled.setCursor(0, y); oled.println(s);
}

void display_status(const String& l1, const String& l2, const String& l3) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  printLine(0, l1);
  printLine(12, l2);
  printLine(24, l3);
  oled.display();
}

void display_cue(bool on) {
  if (on) {
    oled.fillRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);
  } else {
    oled.clearDisplay();
  }
  oled.display();
}

void display_show_raw(int raw, int minv, int maxv) {
  if (maxv <= minv) maxv = minv + 1;
  if (raw < minv) raw = minv;
  if (raw > maxv) raw = maxv;
  // compute proportion
  float p = (float)(raw - minv) / (float)(maxv - minv);
  int w = (int)(p * (OLED_WIDTH - 2));
  int out8 = map(raw, minv, maxv, 0, 255);

  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0); oled.println("Fader RAW");
  oled.setCursor(0, 12); oled.print("RAW="); oled.println(raw);
  oled.setCursor(0, 24); oled.print("OUT8="); oled.println(out8);

  // draw bar at y=32
  oled.drawRect(0, 32, OLED_WIDTH, 10, SSD1306_WHITE);
  if (w > 0) {
    oled.fillRect(1, 33, w, 8, SSD1306_WHITE);
  }
  oled.display();
}


void display_show_fader_and_dir(int raw, int minv, int maxv, const String& dir, int out8) {
  if (maxv <= minv) maxv = minv + 1;
  if (raw < minv) raw = minv;
  if (raw > maxv) raw = maxv;
  float ratio = (float)(raw - minv) / (float)(maxv - minv);
  if (ratio < 0) ratio = 0; if (ratio > 1) ratio = 1;
  int bar_w = (int)(ratio * (OLED_WIDTH - 2));

  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("T2 "); oled.print(dir);
  oled.setCursor(0, 10);
  oled.print("RAW="); oled.print(raw);
  oled.setCursor(0, 20);
  oled.print("OUT8="); oled.print(out8);
  oled.drawRect(0, 32, OLED_WIDTH, 12, SSD1306_WHITE);
  if (bar_w > 0) oled.fillRect(1, 33, bar_w, 10, SSD1306_WHITE);
  oled.display();
}
