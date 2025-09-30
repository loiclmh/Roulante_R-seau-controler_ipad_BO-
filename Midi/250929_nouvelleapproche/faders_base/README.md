# Faders Base (RP2040)

Un projet minimal **faders + filtre** pour RP2040 (Pico / Seeeduino XIAO RP2040).  
Objectif : une base propre, sans dépendances exotiques, avec un filtrage façon Control Surface et (optionnel) envoi de **USB‑MIDI CC**.

## C câblage
- `FADER_PINS` dans le code : par défaut `{26,27,28,29}` (entrées ADC).  
- Masse des faders → GND, curseur → pin ADC, extrémités → 3V3 et GND.

## Filtrage
- Filtre exponentiel (style Control Surface) : `filtered += (raw - filtered) >> FILTER_SHIFT;`
- `FILTER_SHIFT` 3..6 (4 par défaut) → plus grand = plus lisse, plus lent.
- Hystérésis sur la sortie 7 bits `HYST` pour éviter le flicker.

## Mapping
- Plage utile `USABLE_MIN` / `USABLE_MAX` (par défaut 40..4025) pour ignorer les butées mécaniques.
- Conversion 0..127.

## MIDI
- Si la lib **Adafruit TinyUSB** est installée, les valeurs sont envoyées en **USB‑MIDI CC** (canal `MIDI_CHANNEL`, CC `FIRST_CC + i`).  
- Sinon, les valeurs sont simplement affichées sur le **Serial Monitor**.

## Compilation
- Carte : *Raspberry Pi Pico* ou *Seeeduino XIAO RP2040* (core Earle Philhower ou Seeed).  
- **Optionnel** : installer *Adafruit TinyUSB Library* pour l’USB‑MIDI.
- Ouvrez `faders_base.ino` et téléversez.

## Paramètres en haut du fichier
- `NUM_FADERS`, `FADER_PINS[]`, `FILTER_SHIFT`, `HYST`, `USABLE_MIN/MAX`, `FIRST_CC`, `MIDI_CHANNEL`.

Bonne base pour itérer vers la console !
