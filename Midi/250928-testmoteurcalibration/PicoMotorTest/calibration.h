// --- Paramètres de filtrage et stabilité ---
const int FADER_NS_CAL = 150;   // nombre d’échantillons pour la moyenne (suréchantillonnage)
const int FADER_HYST_CAL = 25;  // hystérésis pour éviter le jitter visuel (plage ~6 counts)

// --- Bornes utilisables pour mapping sûr ---
const int FADER_USABLE_MIN_CAL = 40;   // zone morte basse
const int FADER_USABLE_MAX_CAL = 4020; // zone morte haute


