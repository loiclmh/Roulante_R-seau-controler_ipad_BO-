#pragma once
#include <Arduino.h>
void display_begin();
void display_status(const String& l1, const String& l2 = "", const String& l3 = "");
// Indicateur plein écran (pour d'autres tests): ON = tout blanc, OFF = noir
void display_cue(bool on);

// Affiche RAW + barre (0..4095)
void display_show_raw(int raw, int minv = 0, int maxv = 4095);

void display_show_fader_and_dir(int raw, int minv, int maxv, const String& dir, int out8);
