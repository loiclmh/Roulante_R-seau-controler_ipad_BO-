#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ======= Réglages de test =======
constexpr uint8_t TEST_CC   = 16;       // numéro de CC à écouter
constexpr auto     TEST_CH   = Channel_1; // canal à écouter (met Channel_2 si besoin)
constexpr uint8_t OLED_ADDR  = 0x3C;    // 0x3C ou 0x3D selon ton écran

// ======= OLED =======
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ======= MIDI =======
USBMIDI_Interface midi;
CCValue ccIn{ MIDIAddress{ TEST_CC, TEST_CH } };

// ======= LED intégrée (Pico GP25) =======
constexpr uint8_t PIN_LED = 25;

// ======= Variables diag =======
uint8_t  lastVal = 0;         // 0..127
uint32_t rxCount = 0;         // nombre de trames CC reçues
uint32_t lastRxMs = 0;        // millis() de la dernière réception
bool     rxBlink = false;     // pour clignoter la LED brièvement
uint32_t lastUiMs = 0;

void setup() {
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  // I2C (OLED sur SDA=GP4, SCL=GP5)
  Wire.begin();
  display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR);
  display.clearDisplay();

  // MIDI
  Control_Surface.begin();

  // Écran d’accueil
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("MIDI RX DIAG (CC)"));
  display.print(F("Listening CC#")); display.print(TEST_CC);
  display.print(F("  Ch"));
  display.println((int)TEST_CH); // Channel_1 s'affiche "1"
  display.display();
}

void loop() {
  Control_Surface.loop();

  // Réception CC
  if (ccIn.getDirty()) {
    lastVal = ccIn.getValue();    // 0..127
    rxCount++;
    lastRxMs = millis();
    rxBlink = true;               // flash LED
    ccIn.clearDirty();
  }

  // LED: flash court à la réception, s’éteint après 60 ms
  if (rxBlink && (millis() - lastRxMs) > 60) {
    rxBlink = false;
  }
  digitalWrite(PIN_LED, rxBlink ? HIGH : LOW);

  // Rafraîchissement OLED toutes ~50 ms
  if (millis() - lastUiMs > 50) {
    lastUiMs = millis();
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println(F("MIDI RX DIAG (CC)"));

    display.setCursor(0, 12);
    display.print(F("CC#")); display.print(TEST_CC);
    display.print(F("  Ch")); display.println((int)TEST_CH);

    display.setCursor(0, 24);
    display.print(F("Last: ")); display.println((int)lastVal);

    display.setCursor(0, 34);
    display.print(F("Count: ")); display.println((unsigned long)rxCount);

    display.setCursor(0, 44);
    uint32_t age = millis() - lastRxMs;
    display.print(F("Age(ms): ")); display.println((unsigned long)age);

    // Barre 0..127
    display.drawRect(0, 54, 120, 10, SSD1306_WHITE);
    int bar = map(lastVal, 0, 127, 0, 120);
    display.fillRect(0, 54, bar, 10, SSD1306_WHITE);

    display.display();
  }
}