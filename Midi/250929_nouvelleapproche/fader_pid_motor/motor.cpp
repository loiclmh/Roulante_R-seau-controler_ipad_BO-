#include <Arduino.h> 
#include "motor.h"
#include "fader_filtre_adc.h"
#include "pid.h"


void setupmotor() {
    for (uint8_t i=0; i<NUM_MOTOR; ++i) {
    pinMode(motors[i]._in1, OUTPUT);
    pinMode(motors[i]._in2, OUTPUT);
    // sécurité au boot : roue libre
    digitalWrite(motors[i]._in1, LOW);
    digitalWrite(motors[i]._in2, LOW);
    analogWriteFreq(freqMotor);
  }
}

//================ENVOI INFO MOTOR =============
void loopmotor(uint8_t i) {
    if (i >= NUM_MOTOR ) return ; 
    // limitation de la consigne pour ne pas dépasser 10V
    int16_t u = constrain(Dirmotor[i], -255L*breakv , 255L*breakv );
    uint8_t pwm = (uint8_t)abs(u);

    if (u == 0) {
        if (nfreinactif[i] >= freinActif ) { // freinage désactivé silence 
            digitalWrite(motors[i]._in1, LOW);
            digitalWrite(motors[i]._in2, LOW);
        return ; 
        }
        else {                               // freinage actif activé
            digitalWrite(motors[i]._in1, HIGH);
            digitalWrite(motors[i]._in2, HIGH);
            nfreinactif[i] = nfreinactif[i] + 1;
        }
    return;
  }

    if (u > 0) { // si inversion des sens changer > par <
      // Avant : IN1 = PWM, IN2 = 0
      digitalWrite(motors[i]._in2, LOW);
      analogWrite (motors[i]._in1, pwm);
      nfreinactif[i] = 0;
    } else {
      // Arrière : IN1 = 0, IN2 = PWM
      digitalWrite(motors[i]._in1, LOW);
      analogWrite (motors[i]._in2, pwm);
      nfreinactif[i] = 0;
  }
}
