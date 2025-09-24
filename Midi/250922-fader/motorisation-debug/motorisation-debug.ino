// ====== Motorized Fader (Pico + DRV8871 IN1/IN2 + OLED + MIDI auto-RX) ======
// Réglages anti-rebond : Kp=0.10, Ki=0, Deadband=50, MAX_PWM=120, freinage actif.
// TX: CC#16 Ch1 (MIDI learn côté logiciel)
// RX: CC#16 ch(1..16) auto-détecté (feedback -> moteur)
//
// Câblage :
// OLED I2C -> SDA=GP4, SCL=GP5 (0x3C / 0x3D)
// Fader -> wiper=A0 (GP26), extrémités 3V3/GND
// DRV8871 -> IN1=GP14, IN2=GP15, VM=9..12V, GND commun, moteur sur OUT1/OUT2
// Boutons (pull-up interne, appui = vers GND) : OK=GP18, UP=GP19, DOWN=GP20

#include <Control_Surface.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET   -1
#define OLED_ADDR_DEFAULT 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- PINS ----------
constexpr uint8_t PIN_FADER_ADC = A0; // GP26
constexpr uint8_t PIN_IN1       = 14; // GP14 -> IN1 (PWM A)
constexpr uint8_t PIN_IN2       = 15; // GP15 -> IN2 (PWM B)
constexpr uint8_t PIN_BTN_OK    = 18;
constexpr uint8_t PIN_BTN_UP    = 19;
constexpr uint8_t PIN_BTN_DOWN  = 20;

// ---------- MIDI ----------
USBMIDI_Interface midi;
constexpr uint8_t CC_NUM = 16; // CC fixe
// TX fixe sur CH1 (idéal pour MIDI learn)
CCPotentiometer potOut { PIN_FADER_ADC, { CC_NUM, Channel_1 }, };
// RX auto: écoute CC16 sur les 16 canaux et utilise le dernier reçu
CCValue ccIn[16] = {
  MIDIAddress{CC_NUM, Channel_1},  MIDIAddress{CC_NUM, Channel_2},
  MIDIAddress{CC_NUM, Channel_3},  MIDIAddress{CC_NUM, Channel_4},
  MIDIAddress{CC_NUM, Channel_5},  MIDIAddress{CC_NUM, Channel_6},
  MIDIAddress{CC_NUM, Channel_7},  MIDIAddress{CC_NUM, Channel_8},
  MIDIAddress{CC_NUM, Channel_9},  MIDIAddress{CC_NUM, Channel_10},
  MIDIAddress{CC_NUM, Channel_11}, MIDIAddress{CC_NUM, Channel_12},
  MIDIAddress{CC_NUM, Channel_13}, MIDIAddress{CC_NUM, Channel_14},
  MIDIAddress{CC_NUM, Channel_15}, MIDIAddress{CC_NUM, Channel_16},
};

// ---------- Contrôle ----------
uint32_t kickUntilMs = 0;
const int KICK_PWM = 55;       // petit coup pour décoller
const int KICK_MS  = 35;       // 35 ms suffit en général
int lastSign = 0;              // mémorise le sens précédent

constexpr int ADC_MAX = 4095;
// *** PWM plafonnée pour calmer la mécanique ***
constexpr int MAX_PWM = 120;
const     int MARGIN  = 40;   // anti-butée mécanique

int   deadband     = 50;   // large pour silence en cible
int   minPWM       = 30;   // seuil de démarrage cohérent
bool  motorEnabled = true;
bool  invertMotor  = false;

// PI (réglables via menu)
int   kp_x10   = 1;   // Kp = 0.10
int   ki_x1000 = 0;   // Ki = 0.000
float integral = 0.0f;

// Cible / mesure
int   targetADC_demand = 0; // demande de consigne (avant rampe)
int   targetADC        = 0; // consigne réelle (après rampe)
int   lastPos   = 0, lastErr = 0, lastPWM = 0;

// Filtrage position (adoucir l’ADC)
float posEMA = 0.0f;
const float alphaPos = 0.12f;  // 0..1 (plus petit = plus lissé)

// Rampe de consigne (adoucir la cible)
const float alphaTarget = 0.18f; // 0..1 (plus petit = plus doux)

// Debug MIDI RX
int      fbChan   = -1;     // dernier canal RX détecté (1..16)
int      lastFB   = -1;     // dernière valeur (0..127)
uint32_t lastFBms = 0;

// Direction affichage
enum class MotorDir : uint8_t { Stopped=0, Forward, Reverse };
MotorDir  motorDir = MotorDir::Stopped;
uint32_t  lastDirChangeMs = 0;

// Source / Menu
enum class SourceMode : uint8_t { FEEDBACK=0, LOCAL=1 };
SourceMode sourceMode = SourceMode::LOCAL;
uint8_t    localValue = 64; // 0..127

enum class MenuItem : uint8_t {
  KP=0, KI, DEADBAND, MIN_PWM, MOTOR_ENABLE, INVERT_MOTOR, SOURCE, LOCAL_VALUE, _COUNT
};
MenuItem currentItem = MenuItem::KP;

// Boutons
struct Btn { uint8_t pin; bool state; bool lastStable; uint32_t lastChangeMs; };
Btn btnUp{PIN_BTN_UP,true,true,0}, btnDown{PIN_BTN_DOWN,true,true,0}, btnOk{PIN_BTN_OK,true,true,0};
bool readButtonEdge(Btn &b){
  uint32_t now=millis(); bool raw=digitalRead(b.pin);
  if(raw!=b.lastStable && (now-b.lastChangeMs)>25){
    b.lastStable=raw; b.lastChangeMs=now;
    if(b.state==HIGH && raw==LOW){ b.state=raw; return true; }
    b.state=raw;
  }
  return false;
}
uint32_t okPressStart=0; bool okHeld=false;

// ================== MOTEUR ==================
inline void motorStop(){
  // Freinage actif bref (inhibe la roue libre → moins de rebond)
  digitalWrite(PIN_IN1, HIGH);
  digitalWrite(PIN_IN2, HIGH);
  delayMicroseconds(2000); // 2 ms de frein
  digitalWrite(PIN_IN1, LOW);
  digitalWrite(PIN_IN2, LOW);
  if (motorDir != MotorDir::Stopped){
    motorDir = MotorDir::Stopped;
    lastDirChangeMs = millis();
  }
}

inline void motorDrive(int pwm, bool dirPositive){
  pwm = constrain(pwm,0,MAX_PWM);
  if(!motorEnabled || pwm==0){ motorStop(); return; }
  bool dir = dirPositive ^ invertMotor;
  if(dir){
    analogWrite(PIN_IN1,pwm); digitalWrite(PIN_IN2,LOW);
    if(motorDir!=MotorDir::Forward){ motorDir=MotorDir::Forward; lastDirChangeMs=millis(); }
  } else {
    digitalWrite(PIN_IN1,LOW); analogWrite(PIN_IN2,pwm);
    if(motorDir!=MotorDir::Reverse){ motorDir=MotorDir::Reverse; lastDirChangeMs=millis(); }
  }
}

void motorJogTest(){
  bool pe=motorEnabled; motorEnabled=true;
  motorDrive(110,true); delay(500); motorStop(); delay(300);
  motorDrive(110,false); delay(500); motorStop(); delay(300);
  motorEnabled=pe;
}

// ================== OLED ==================
void drawUI(){
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  display.setTextWrap(false);

  int T7 = map(targetADC,0,ADC_MAX,0,127);
  int P7 = map(lastPos,  0,ADC_MAX,0,127);

  // L1 T/P
  display.setTextSize(2);
  display.setCursor(0,0);
  display.print(F("T")); display.print(T7);
  display.print(F(" P")); display.print(P7);

  // L2 Err/PWM + menu tag
  display.setTextSize(1);
  display.setCursor(0,28);
  display.print(F("E:")); display.print(lastErr);
  display.print(F("  W:")); display.print(lastPWM);
  display.print(F("  ["));
  switch(currentItem){
    case MenuItem::KP:           display.print(F("Kp"));  break;
    case MenuItem::KI:           display.print(F("Ki"));  break;
    case MenuItem::DEADBAND:     display.print(F("DB"));  break;
    case MenuItem::MIN_PWM:      display.print(F("Min")); break;
    case MenuItem::MOTOR_ENABLE: display.print(F("EN"));  break;
    case MenuItem::INVERT_MOTOR: display.print(F("INV")); break;
    case MenuItem::SOURCE:       display.print(F("SRC")); break;
    case MenuItem::LOCAL_VALUE:  display.print(F("LOC")); break;
    default: break;
  }
  display.print(F("]"));

  // L3 DIR
  display.setTextSize(2);
  display.setCursor(0,44);
  display.print(F("DIR: "));
  switch(motorDir){
    case MotorDir::Forward: display.print(F("FWD")); break;
    case MotorDir::Reverse: display.print(F("REV")); break;
    default:                display.print(F("STOP")); break;
  }
  if (millis()-lastDirChangeMs < 400) display.fillCircle(124,6,3,SSD1306_WHITE);

  // L4 debug MIDI RX
  display.setTextSize(1);
  display.setCursor(0,36);
  display.print(F("FB: "));
  if (lastFB >= 0) { display.print(lastFB); } else { display.print(F("--")); }
  display.print(F(" ch"));
  if (fbChan > 0)  display.print(fbChan); else display.print(F("-"));
  if (millis()-lastFBms < 300) display.print(F(" *"));

  // Valeur de l'item (haut droite)
  display.setCursor(78,16);
  switch(currentItem){
    case MenuItem::KP:           display.print(F("Kp=")); display.print(kp_x10/10.0f,1); break;
    case MenuItem::KI:           display.print(F("Ki=")); display.print(ki_x1000/1000.0f,3); break;
    case MenuItem::DEADBAND:     display.print(F("DB=")); display.print(deadband); break;
    case MenuItem::MIN_PWM:      display.print(F("Min=")); display.print(minPWM); break;
    case MenuItem::MOTOR_ENABLE: display.print(F("EN=")); display.print(motorEnabled?F("ON"):F("OFF")); break;
    case MenuItem::INVERT_MOTOR: display.print(F("INV=")); display.print(invertMotor?F("ON"):F("OFF")); break;
    case MenuItem::SOURCE:       display.print(F("SRC=")); display.print(sourceMode==SourceMode::FEEDBACK?F("FB"):F("LOC")); break;
    case MenuItem::LOCAL_VALUE:  display.print(F("LOC=")); display.print((int)localValue); break;
    default: break;
  }

  display.display();
}

// ================== MENU ==================
void nextMenuItem(){
  uint8_t i=(uint8_t)currentItem;
  i=(i+1)% (uint8_t)MenuItem::_COUNT;
  currentItem=(MenuItem)i;
}

void adjustCurrentItem(int d){
  switch(currentItem){
    case MenuItem::KP:           kp_x10    = constrain(kp_x10 + d, 1, 300); break;
    case MenuItem::KI:           ki_x1000  = constrain(ki_x1000 + d*5, 0, 1000); break;
    case MenuItem::DEADBAND:     deadband  = constrain(deadband + d, 0, 100); break;
    case MenuItem::MIN_PWM:      minPWM    = constrain(minPWM + d*5, 0, 255); break;
    case MenuItem::MOTOR_ENABLE: if(d!=0) motorEnabled=!motorEnabled; break;
    case MenuItem::INVERT_MOTOR: if(d!=0) invertMotor=!invertMotor; break;
    case MenuItem::SOURCE:       if(d!=0) sourceMode=(sourceMode==SourceMode::FEEDBACK)?SourceMode::LOCAL:SourceMode::FEEDBACK; break;
    case MenuItem::LOCAL_VALUE:  localValue= constrain((int)localValue + d, 0, 127); break;
    default: break;
  }
}

// ================== SETUP / LOOP ==================
void setup(){
  Wire.setSDA(4); Wire.setSCL(5); Wire.setClock(100000); Wire.begin();
  delay(200);
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR_DEFAULT)) display.begin(SSD1306_SWITCHCAPVCC, 0x3D);
  display.clearDisplay(); display.setTextSize(2); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0); display.println(F("OLED OK")); display.display(); delay(300);

  pinMode(PIN_IN1,OUTPUT); pinMode(PIN_IN2,OUTPUT); motorStop();
  pinMode(PIN_BTN_OK,INPUT_PULLUP); pinMode(PIN_BTN_UP,INPUT_PULLUP); pinMode(PIN_BTN_DOWN,INPUT_PULLUP);

  analogReadResolution(12);
  Control_Surface.begin();

  // Consigne initiale = position (évite le saut)
  int pos0 = analogRead(PIN_FADER_ADC);
  targetADC = targetADC_demand = constrain(pos0, MARGIN, ADC_MAX - MARGIN);
  posEMA = pos0;
}

void loop(){
  Control_Surface.loop(); // TX potOut + RX possibles

  // Boutons + JOG (OK long)
  bool up = readButtonEdge(btnUp);
  bool dn = readButtonEdge(btnDown);
  bool ok = readButtonEdge(btnOk);
  if(ok){ nextMenuItem(); okPressStart=millis(); okHeld=true; }
  if(okHeld && digitalRead(PIN_BTN_OK)==LOW && (millis()-okPressStart)>1000){ okHeld=false; motorJogTest(); }
  if(digitalRead(PIN_BTN_OK)==HIGH){ okHeld=false; }
  if(up) adjustCurrentItem(+1);
  if(dn) adjustCurrentItem(-1);

  // RX MIDI auto (si FEEDBACK), sinon LOCAL
  if (sourceMode==SourceMode::FEEDBACK) {
    for (int i=0;i<16;i++){
      if (ccIn[i].getDirty()){
        uint8_t v = ccIn[i].getValue();
        lastFB = v; fbChan = i+1; lastFBms = millis();
        targetADC_demand = map(v,0,127,0,ADC_MAX);
        ccIn[i].clearDirty();
      }
    }
  } else {
    targetADC_demand = map((int)localValue,0,127,0,ADC_MAX);
  }
  targetADC_demand = constrain(targetADC_demand, MARGIN, ADC_MAX - MARGIN);

  // Rampe de consigne (adoucir) : cible réelle glisse vers la demande
  targetADC += (int)((targetADC_demand - targetADC) * alphaTarget);

  // Mesure (filtrée)
  int posRaw = analogRead(PIN_FADER_ADC);
  posEMA += (posRaw - posEMA) * alphaPos;
  int pos = (int)(posEMA + 0.5f);

  int err = targetADC - pos;

  // Anti-windup / proche cible
  static int prevErr = 0;
  if (abs(err) <= deadband || (err > 0 && prevErr < 0) || (err < 0 && prevErr > 0)) {
    integral = 0.0f;
  }
  prevErr = err;

  if(!motorEnabled){
    motorStop(); integral = 0.0f; lastPWM = 0;

  } else if (abs(err) <= deadband){
    motorStop(); integral = 0.0f; lastPWM = 0;
    // (optionnel) réaligner un peu la cible pour éviter la chasse
    targetADC = (targetADC*3 + pos)/4;

  } else {
    // ---- calcul PI de base ----
    integral += err;
    integral = constrain(integral, -4000.0f, 4000.0f);

    float Kp = kp_x10 / 10.0f;
    float Ki = ki_x1000 / 1000.0f;
    float u  = Kp * err + Ki * integral;   // commande "signée"

    int sgn = (u > 0) - (u < 0);           // +1 si u>0, -1 si u<0, 0 sinon

    // ---- Détection redémarrage / changement de sens ----
    bool restarting = (lastPWM == 0 && sgn != 0);     // on repart de l'arrêt
    bool dirChange  = (sgn != 0 && sgn != lastSign);  // inversion du sens

    if ((restarting || dirChange) && sgn != 0) {
      kickUntilMs = millis() + KICK_MS;               // lance le "kick"
    }

    int pwmVal;
    if (millis() < kickUntilMs) {
      pwmVal = KICK_PWM;                               // boost court
    } else {
      pwmVal = (int)abs(u);                            // commande normale
      if (pwmVal > 0 && pwmVal < minPWM) pwmVal = minPWM;
      pwmVal = constrain(pwmVal, 0, MAX_PWM);
    }

    motorDrive(pwmVal, (sgn > 0));
    lastPWM = pwmVal;
    if (sgn != 0) lastSign = sgn;
  }

  lastPos = pos; lastErr = err;
  drawUI();
}