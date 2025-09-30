
/*
  Home Security Device (Arduino)
  Components:
    - PIR motion sensor (D2)
    - Magnetic reed door sensor (D3, to GND with INPUT_PULLUP)
    - Arm/Disarm button (D4, hold 2s to toggle)
    - RGB LED: R=D5, G=D6  (you can add B on another PWM if desired)
    - Buzzer (D10)

  States:
    DISARMED → EXIT_DELAY → ARMED → ENTRY_DELAY → ALARM

  Non-blocking timing via millis(); debounced inputs.
*/

#include <Arduino.h>

// Pins
const uint8_t PIN_PIR    = 2;
const uint8_t PIN_REED   = 3;   // to GND, uses INPUT_PULLUP
const uint8_t PIN_BTN    = 4;   // to GND, uses INPUT_PULLUP
const uint8_t PIN_LED_R  = 5;   // PWM
const uint8_t PIN_LED_G  = 6;   // PWM
const uint8_t PIN_BUZZER = 10;  // PWM capable

// Config
const unsigned long EXIT_DELAY_MS  = 15000UL;
const unsigned long ENTRY_DELAY_MS = 10000UL;
const unsigned long ALARM_TIME_MS  = 60000UL;
const bool REED_NC = false; // true if reed is Normally-Closed (invert logic)

// Debounce helper
struct Debounced {
  uint8_t pin; bool state; bool stable; unsigned long last; unsigned long ms;
};

void dbInit(Debounced &d, uint8_t pin, unsigned long ms = 30) {
  d.pin = pin; d.ms = ms; d.state = digitalRead(pin); d.stable = d.state; d.last = millis();
}
bool dbTick(Debounced &d) {
  bool raw = digitalRead(d.pin);
  if (raw != d.state) { d.state = raw; d.last = millis(); }
  if (millis() - d.last > d.ms && d.stable != d.state) { d.stable = d.state; return true; }
  return false;
}

// LED helpers
void ledRGB(uint8_t r, uint8_t g) {
  analogWrite(PIN_LED_R, r);
  analogWrite(PIN_LED_G, g);
}
void ledOff(){ ledRGB(0,0); }
void ledGreen(){ ledRGB(0,180); }
void ledRed(){ ledRGB(180,0); }
void ledYellow(){ ledRGB(160,120); }

// Simple beepers
void beepPattern(unsigned long onMs, unsigned long offMs, uint8_t reps, unsigned int freq=2000){
  for(uint8_t i=0;i<reps;i++){
    tone(PIN_BUZZER, freq);
    delay(onMs);
    noTone(PIN_BUZZER);
    if(i+1<reps) delay(offMs);
  }
}

struct Beeper { bool active=false; unsigned long iv=600, last=0; bool on=false; unsigned int f=2200; unsigned long onW=70; };
Beeper exitBeeper, entryBeeper;
void beeperTick(Beeper &b){
  if(!b.active) return;
  unsigned long now=millis();
  if(now - b.last >= b.iv){ b.last = now; b.on = true; tone(PIN_BUZZER, b.f); }
  if(b.on && now - b.last >= b.onW){ b.on = false; noTone(PIN_BUZZER); }
}

// State machine
enum Mode { DISARMED, EXIT_DELAY, ARMED, ENTRY_DELAY, ALARM };
Mode mode = DISARMED; unsigned long modeStart=0, alarmStart=0;

// Inputs
Debounced btn, reed, pir;
const unsigned long HOLD_MS = 1500; unsigned long holdStart=0; bool holding=false;

void enterMode(Mode m){
  mode = m; modeStart = millis();
  switch(mode){
    case DISARMED: ledGreen(); noTone(PIN_BUZZER); exitBeeper.active=false; entryBeeper.active=false; beepPattern(60,60,2); break;
    case EXIT_DELAY: ledYellow(); exitBeeper={true,800,0,false,1800,60}; entryBeeper.active=false; break;
    case ARMED: ledRed(); exitBeeper.active=false; entryBeeper.active=false; beepPattern(40,40,3); break;
    case ENTRY_DELAY: ledYellow(); entryBeeper={true,400,0,false,2400,80}; break;
    case ALARM: ledRed(); exitBeeper.active=false; entryBeeper.active=false; alarmStart=millis(); break;
  }
}

bool reedOpen(){
  // INPUT_PULLUP: LOW when closed to GND; if NC, invert semantics.
  bool raw = reed.stable; 
  return REED_NC ? !raw : raw;
}
bool pirMotion(){ return pir.stable; }

void handleButtonHold(){
  bool pressed = (btn.stable == LOW);
  if(pressed && !holding){ holding=true; holdStart=millis(); }
  else if(!pressed && holding){ holding=false; }
  if(holding && millis()-holdStart >= HOLD_MS){
    holding=false;
    if(mode==DISARMED) enterMode(EXIT_DELAY);
    else enterMode(DISARMED);
  }
}

void alarmTick(){
  if(mode!=ALARM) return;
  int f = 1000 + (int)(500 * sin(millis()/120.0));
  tone(PIN_BUZZER, f);
  if(millis() - alarmStart >= ALARM_TIME_MS){
    alarmStart = millis(); // loop alarm
  }
}

void setup(){
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_REED, INPUT_PULLUP);
  pinMode(PIN_BTN, INPUT_PULLUP);
  pinMode(PIN_LED_R, OUTPUT);
  pinMode(PIN_LED_G, OUTPUT);
  pinMode(PIN_BUZZER, OUTPUT);

  dbInit(btn, PIN_BTN);
  dbInit(reed, PIN_REED);
  dbInit(pir, PIN_PIR);

  ledOff();
  enterMode(DISARMED);
}

void loop(){
  dbTick(btn); dbTick(reed); dbTick(pir);
  handleButtonHold();
  beeperTick(exitBeeper); beeperTick(entryBeeper);

  unsigned long now = millis();
  switch(mode){
    case DISARMED: break;
    case EXIT_DELAY: if(now - modeStart >= EXIT_DELAY_MS) enterMode(ARMED); break;
    case ARMED: if(reedOpen() || pirMotion()) enterMode(ENTRY_DELAY); break;
    case ENTRY_DELAY: if(now - modeStart >= ENTRY_DELAY_MS) enterMode(ALARM); break;
    case ALARM: alarmTick(); break;
  }
}
