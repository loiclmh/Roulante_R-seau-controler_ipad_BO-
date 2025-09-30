#pragma once
#include <Arduino.h>

void display_begin();
void display_status(const String& l1, const String& l2 = "", const String& l3 = "");
void display_fader(int raw, int minv, int maxv, const String& note, int out8);
