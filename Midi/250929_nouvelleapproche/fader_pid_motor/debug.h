#pragma once
#include <cstdint>

extern bool on_debug;
extern bool on_debug_python;
extern bool on_debug_monitorarduino;
extern uint8_t bash_test_mode;
extern int debugOLED_fader;
extern int debug_fadermoniteur;

void debugsetup();
