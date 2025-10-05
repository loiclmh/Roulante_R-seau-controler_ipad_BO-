
#pragma once
#include <cstdint>
#include <cmath>
#include "motor.h"
#include "fader_filtre_adc.h"

//==================== DBUG+MODE TEST =====================

// ===================== "données défini dans d'autre fichier " =====================
// uint16_t gFaderADC[NUM_MOTOR]  // positions réelles => fader_filtre_adc.h
// uint16_t setPosition[NUM_MOTOR]; // consignes => fader+pid+motor.ino
// int16_t  Dirmotor[NUM_MOTOR];    // sortie PID signée => motor.h
// uint8_t bash_test_pid; // active ou non com scrypte python =>has_serial.h

// ===================== PID instances =====================


class PID {
    public:
    PID() = default;

    // ctor utilisé dans ton pid.cpp : PID(kp,ki,kd,Ts,fc,maxOutput)
    PID(float kp, float ki, float kd, float Ts, float f_c = 0, float maxOutput = 255)
        : Ts(Ts), maxOutput(maxOutput) {
        setKp(kp);
        setKi(ki);
        setKd(kd);
        setEMACutoff(f_c);
    }

    // règle la consigne
    void setSetpoint(uint16_t sp) {
        if (setpoint != sp) activityCount = 0;
        setpoint = sp;
    }

    // getters/setters gains
    void  setKp(float v) { kp = v; }
    void  setKi(float v) { ki_Ts = v * Ts; }
    void  setKd(float v) { kd_Ts = (Ts > 0 ? v / Ts : 0); }
    float getKp() const { return kp; }
    float getKi() const { return ki_Ts / Ts; }
    float getKd() const { return kd_Ts * Ts; }
    static constexpr float kPI = 3.14159265358979323846f;

    // filtre dérivé (EMA) – f_c en Hz ; 0 => pas de filtrage
    void setEMACutoff(float f_c) {
        if (f_c <= 0) { emaAlpha = 1; return; }
        float fn = f_c * Ts; // fréquence normalisée
        emaAlpha = calcAlphaEMA(fn);
    }

    // mise à jour : entrée = mesure position (ADC)
    float update(uint16_t meas_y) {
        // erreur
        float error = float(setpoint) - float(meas_y);

        // dérivée via EMA sur l'entrée (style tttapa)
        float diff  = emaAlpha * (prevInput - float(meas_y));
        prevInput  -= diff;

        // anti-sommeil (hystérésis petite bande morte autour de la consigne filtrée)
        if (activityThres && activityCount >= activityThres) {
        float filtError = float(setpoint) - prevInput;
        if (filtError >= -errThres && filtError <= errThres) {
            errThres = 2; // hystérésis
            return 0.0f;
        } else {
            errThres = 1;
        }
        } else {
        ++activityCount;
        errThres = 1;
        }

        // intégrale candidate
        int32_t newIntegral = integral + int32_t(error);

        // PID (P + I + D)
        float u = kp * error + ki_Ts * float(integral) + kd_Ts * diff;

        // saturation + anti-windup simple
        if (u >  maxOutput) u =  maxOutput;
        else if (u < -maxOutput) u = -maxOutput;
        else integral = newIntegral;

        return u; // [-maxOutput .. +maxOutput]
    }

    // options utiles
    void setMaxOutput(float m) { maxOutput = m; }
    float getMaxOutput() const { return maxOutput; }
    void resetIntegral() { integral = 0; }
    void resetActivityCounter() { activityCount = 0; }
    void setActivityTimeout(float s) {
        activityThres = (s <= 0) ? 0 : (uint16_t)(s / Ts);
        if (activityThres == 0 && s > 0) activityThres = 1;
    }

    private:
    // alpha de l’EMA à partir de la fréquence normalisée fn = f_c * Ts
    static float calcAlphaEMA(float fn) {
        if (fn <= 0) return 1.0f;
        const float c = std::cos(2.0f * kPI * fn);
        return c - 1.0f + std::sqrt(c * c - 4.0f * c + 3.0f);
    }

    // paramètres
    float Ts        = 1.0f;
    float maxOutput = 255.0f;
    float kp        = 1.0f;
    float ki_Ts     = 0.0f;   // Ki * Ts
    float kd_Ts     = 0.0f;   // Kd / Ts
    float emaAlpha  = 1.0f;

    // état
    float    prevInput     = 0.0f;
    int32_t  integral      = 0;
    uint16_t setpoint      = 0;
    uint16_t activityCount = 0;
    uint16_t activityThres = 0;
    uint8_t  errThres      = 1;
    };

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
extern int16_t Dirmotor[NUM_MOTOR] ;    // sortie PID signée => motor.h
extern uint16_t setPosition[NUM_MOTOR]; // consignes => fader+pid+motor.ino

// ====================== API =====================
void initial_PIDv(bool use_python); // choisit défaut/python et (re)crée les PID
void pidBegin();                    // (re)crée les PID avec kp/ki/kd/ts/fc courants
void loopPID(uint8_t i);            // met à jour 1 PID (remplit Dirmotor[i])
void pidEnd();                     // détruit les PID (libère la mémoire)



