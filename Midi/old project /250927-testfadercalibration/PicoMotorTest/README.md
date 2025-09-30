# PicoMotorTest — Flat + RAW bytes option
- Test 1 fournit **ADC brut** de deux manières:
- `analogReadResolution(12)` activé si dispo (RP2040 = 12 bits).  
Pas de moyenne interne — mais vous pouvez ajuster ces constantes dans `test1_adc_raw.cpp` pour stabiliser la lecture :

- **NS** : nombre d’échantillons utilisés pour la moyenne (suréchantillonnage). Plus grand = plus stable, mais plus lent.
- **HYST** : hystérésis pour éviter le jitter visuel. Plus grand = moins de variations rapides mais moins précis.
- **USABLE_MIN** : zone morte basse (en counts ADC). Valeur minimale sûre avant de considérer 0%.
- **USABLE_MAX** : zone morte haute (en counts ADC). Valeur maximale sûre avant de considérer 100%.

Changer de test: `Bash_test.h` (TEST=1/2/3).
