Fader-Midi-Pico — MODS PACK
===========================

Ce pack contient les fichiers corrigés :
- display.hpp  (déclarations uniquement)
- pid.h        (PID avec vos valeurs calibrées)
- pid.cpp      (implémentation clean)

Instructions :
1. Remplacez vos fichiers display.hpp, pid.h et pid.cpp par ceux-ci.
2. Dans main.cpp :
   - ajoutez `#include "pid.h"`
   - ajoutez `static PID pid; static bool prevTouched = false;` en global
   - dans setup() après analogReadResolution : `pid.applyDefaults();`
   - dans loop(), remplacez le bloc P simple par l'appel à `pid.update(...)`.
3. Dans midi_io.hpp, utilisez :
   midi.sendControlChange(MIDIAddress{MIDI_CC_NUM, MIDI_CH}, cc);
