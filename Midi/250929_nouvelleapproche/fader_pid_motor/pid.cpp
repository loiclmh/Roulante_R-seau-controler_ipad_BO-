#include "pid.h"
#include "fader_filtre_adc.h"
#include "motor.h"
#include "has_serial.h"

// ======================= Constantes PID (garde fou au cas de pb d'initialisation)====================
        // Valeurs courantes (communes à tous)
        float kp = KP_DEFAUT;
        float ki = KI_DEFAUT;
        float kd = KD_DEFAUT;
        float ts = TS_DEFAUT;
        float fc = FC_DEFAUT;

        // Buffer Python (si tu veux injecter depuis PC)
        float kp_python = KP_DEFAUT;
        float ki_python = KI_DEFAUT;
        float kd_python = KD_DEFAUT;
        float ts_python = TS_DEFAUT;
        float fc_python = FC_DEFAUT;

// Une instance PID par moteur
PID* gPid[NUM_MOTOR] = { nullptr };

// (re)crée les PID avec les valeurs courantes
void pidBegin() {
  for (uint8_t i = 0; i < NUM_MOTOR; ++i) {
    if (gPid[i]) { delete gPid[i]; gPid[i] = nullptr; }
    // ctor: PID(float kp, float ki, float kd, float Ts, float f_c = 0, float maxOutput = 255)
    gPid[i] = new PID(kp, ki, kd, ts, fc, 255.0f);
  }
}

// Choisit défaut/python et applique
void initial_PIDv() {
  if (bash_test_pid == 1) {
    kp = kp_python;  ki = ki_python;  kd = kd_python;
    ts = ts_python;  fc = fc_python;
  } else {
    kp = KP_DEFAUT;  ki = KI_DEFAUT;  kd = KD_DEFAUT;
    ts = TS_DEFAUT;  fc = FC_DEFAUT;
  }
  // ts influe le constructeur -> on (re)crée les PID
  pidBegin();
}

void loopPID(uint8_t i) {
  if (i >= NUM_MOTOR || !gPid[i]) return;
  loopfader(i);
  gPid[i]->setSetpoint(setPosition[i]);     // consigne (0..4095)
  float u = gPid[i]->update(gFaderADC[i]);  // mesure  (0..4095) → float

  // float → int16 signé (−255..+255) arrondi propre
  Dirmotor[i] = (int16_t)((u >= 0.f) ? (u + 0.5f) : (u - 0.5f));
  loopmotor(i);
}