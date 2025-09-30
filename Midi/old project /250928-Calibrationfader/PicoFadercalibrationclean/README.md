# PicoFaderTest (clean)
But: **tester un fader**, afficher les données sur OLED SSD1306 (I2C), et **garder une communication simple** avec un script Python.

## Arborescence
```
PicoFaderTest/
  PicoFaderTest.ino
  Bash_test.h
  pins.h
  calibration.h
  display.h
  display.cpp
  comms.h
  comms.cpp
  tests/
    test1_fader.cpp
    test2_motor_jog.cpp
    test3_pid_control.cpp
  tools/
    host_comms.py
```
- `TEST` se règle dans `Bash_test.h` (1 par défaut).
- Les **constantes** pour le filtrage et le mapping sont **tout en haut** de `tests/test1_fader.cpp` :
  - `NS` (suréchantillonnage), `HYST` (hystérésis),
  - `USABLE_MIN`, `USABLE_MAX` (marges de butée).

## Dépendances Arduino
- Adafruit GFX
- Adafruit SSD1306

## Protocole série (exemples)
- PC → MCU : `PING` → MCU répond `PONG`
- MCU → PC : `RAW=1234` et `OUT8=45` (lignes séparées)
- PC → MCU : `GET` → MCU répond `RAW=... OUT8=...`

## Script Python
Voir `tools/host_comms.py` (nécessite `pyserial`).

## Portage rapide depuis l'ancien projet
- OLED et protocole série sont isolés dans `display.*` et `comms.*`.
- Le test 1 reprend la philosophie "fader + écran" avec un code **simplifié et propre**.
- Les tests 2 et 3 sont des **stubs** en attendant la partie moteur/PID.
