#include <Arduino.h>
#include <Arduino_Helpers.h>
#include <AH/Hardware/FilteredAnalog.hpp>


#include "calibration.h"
#include "fader_info.h"

// Facteur de lissage du filtre Control Surface (3 = plus réactif, 5/6 = plus doux)
constexpr uint8_t FILTER_SHIFT = 4;

extern const uint8_t PIN_FADERS[NUM_FADERS];
static AH::FilteredAnalog<12, FILTER_SHIFT, uint32_t>* gFaders[NUM_FADERS] = {nullptr};

static int map_to_8bit(int raw, int usable_min, int usable_max) {
  if (usable_min > usable_max) {
    int tmp = usable_min; usable_min = usable_max; usable_max = tmp;
  }
  if (raw < usable_min) raw = usable_min;
  if (raw > usable_max) raw = usable_max;
  long span = (long)usable_max - (long)usable_min;
  if (span <= 0) return 0;
  long v = (long)(raw - usable_min) * 255L / span;
  if (v < 0) v = 0; if (v > 255) v = 255;
  return (int)v;
}

void ADC_FADER() {
  static bool inited = false;
  if (!inited) {
    AH::FilteredAnalog<>::setupADC();
    for (int i = 0; i < NUM_FADERS; ++i) {
      gFaders[i] = new AH::FilteredAnalog<12, FILTER_SHIFT, uint32_t>(PIN_FADERS[i]);
      gFaders[i]->resetToCurrentValue();
    }
    inited = true;
  }

  for (int i = 0; i < NUM_FADERS; ++i) {
    gFaders[i]->update();
    int raw = (int)gFaders[i]->getValue();

    int usable_min = gCalib.fader[i]._usable_min;
    int usable_max = gCalib.fader[i]._usable_max;
    int out8 = map_to_8bit(raw, usable_min, usable_max);

    char key[16];
    snprintf(key, sizeof(key), "RAW%d", i);
    comms_kv(key, raw);
    snprintf(key, sizeof(key), "OUT8_%d", i);
    comms_kv(key, out8);
  }

  delay(1);
}