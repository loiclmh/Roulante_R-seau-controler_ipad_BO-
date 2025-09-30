#pragma once

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1


//===================== setup et loop OLED  =========
void setupOLED();
void drawInfo();


//====================  Fonctions OLED  ========================
void drawOLED ();
void drawOLED(int value);
