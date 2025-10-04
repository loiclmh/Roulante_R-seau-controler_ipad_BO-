// pid.cpp
#include <Arduino.h>         // pour uint8_t, millis, etc. (rp2040 core)
#include "pid.h"             // contient la définition complète de class PID
#include "motor.h"           // NUM_MOTOR et loopmotor()
#include "fader_filtre_adc.h"// gFaderADC[] si besoin ailleurs
#include "debug.h"           // on_debug, on_debug_python, bash_test_mode (si tu les utilises)
                             
// ======================= Variables “courantes” PID =======================
// (définies ici, déclarées en 'extern' dans pid.h)
float kp = KP_DEFAUT;
float ki = KI_DEFAUT;
float kd = KD_DEFAUT;
float ts = TS_DEFAUT;
float fc = FC_DEFAUT;

// Valeurs injectables par Python (définies ici, extern dans pid.h)
float kp_python = KP_DEFAUT;
float ki_python = KI_DEFAUT;
float kd_python = KD_DEFAUT;
float ts_python = TS_DEFAUT;
float fc_python = FC_DEFAUT;

// Une instance PID par moteur (définition ici, extern dans pid.h)
PID* gPid[NUM_MOTOR] = { nullptr };

// Ces deux tableaux sont “extern” dans pid.h → on les DÉFINIT ici
int16_t  Dirmotor[NUM_MOTOR]    = {0};
uint16_t setPosition[NUM_MOTOR] = {0};

// ------------------------------------------------------------------------
// (re)crée les PID avec les valeurs courantes kp/ki/kd/ts/fc
void pidBegin() {
  // NOTE : si tu as une classe PID dans un autre .h, assure-toi que pid.h
  // contient sa définition complète (ce que tu as fait), pas juste “class PID;”.
  for (uint8_t i = 0; i < NUM_MOTOR; ++i) {
    if (gPid[i]) { delete gPid[i]; gPid[i] = nullptr; }
    // ctor: PID(float kp, float ki, float kd, float Ts, float f_c=0, float max=255)
    gPid[i] = new PID(kp, ki, kd, ts, fc, 255.0f);
  }
}

// Choisit défaut/python, puis (re)crée les objets PID avec ces valeurs
void initial_PIDv(bool use_python) {
  if (use_python) {
    kp = kp_python;  ki = ki_python;  kd = kd_python;
    ts = ts_python;  fc = fc_python;
  } else {
    kp = KP_DEFAUT;  ki = KI_DEFAUT;  kd = KD_DEFAUT;
    ts = TS_DEFAUT;  fc = FC_DEFAUT;
  }
  pidBegin();
}

// Met à jour le PID d’un moteur i et remplit Dirmotor[i]
// (pense à appeler loopmotor(i) ensuite, dans ta boucle principale OU ici si tu préfères)
void loopPID(uint8_t i) {
  if (i >= NUM_MOTOR) return;
  if (!gPid[i]) return;

  // consigne et mesure
  gPid[i]->setSetpoint(setPosition[i]);      // 0..4095
  float u = gPid[i]->update(/*meas*/ gFaderADC[i]); // float [-max..+max]

  // float → int16_t avec arrondi “propre”
  Dirmotor[i] = (int16_t)((u >= 0.f) ? (u + 0.5f) : (u - 0.5f));
}
void pidEnd() {
  for (uint8_t i = 0; i < NUM_MOTOR; ++i) {
    if (gPid[i]) { delete gPid[i]; gPid[i] = nullptr; }
  }
}