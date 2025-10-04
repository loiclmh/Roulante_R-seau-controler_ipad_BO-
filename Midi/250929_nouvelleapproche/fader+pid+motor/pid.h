
#pragma once




/**/
//===================== DBUG+MODE TEST =====================



// ===================== "données défini dans d'autre fichier " =====================
// uint16_t gFaderADC[NUM_MOTOR]  // positions réelles => fader_filtre_adc.h
// uint16_t setPosition[NUM_MOTOR]; // consignes => fader+pid+motor.ino
// int16_t  Dirmotor[NUM_MOTOR];    // sortie PID signée => motor.h
// uint8_t bash_test_pid; // active ou non com scrypte python =>has_serial.h

// ===================== PID instances =====================
class PID;                 // forward declaration
extern PID* gPid[NUM_MOTOR];

// ======================= Constantes PID ====================

   // Valeurs par défaut
constexpr float KP_DEFAUT = 1.00f;
constexpr float KI_DEFAUT = 0.20f;
constexpr float KD_DEFAUT = 0.05f;
constexpr float TS_DEFAUT = 0.001f;   // 1 kHz
constexpr float FC_DEFAUT = 10.0f;    // Hz

    // Valeurs courantes utilisées par ton PID
extern float kp, ki, kd, ts, fc;

    // Valeurs injectable par python
extern float kp_python, ki_python, kd_python, ts_python, fc_python;

// ======================= constantes externes =====================
extern int16_t Dirmotor[NUM_MOTOR];    // sortie PID signée => motor.h


// ====================== API =====================
void initial_PIDv(bool use_python); // choisit défaut/python et (re)crée les PID
void pidBegin();                    // (re)crée les PID avec kp/ki/kd/ts/fc courants
void loopPID(uint8_t i);            // met à jour 1 PID (remplit Dirmotor[i])
void loopPIDAll();                  // pratique : met à jour tous




