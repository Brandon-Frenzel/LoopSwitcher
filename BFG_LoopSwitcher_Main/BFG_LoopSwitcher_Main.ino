/*
  Project for relay-based true bypass programmable loop switch for guitar pedal effects.
  Created by Brandon A. Frenzel, April 3, 2019.
  //TODO: copyright information.
*/

#include "MIDI.h"

//MIDI_CREATE_DEFAULT_INSTANCE();
MIDI_CREATE_INSTANCE(HardwareSerial, Serial5, MIDI); //initialize a MIDI serial connection

#define RELAY1_A 14
#define RELAY1_B 15
#define RELAY2_A 16
#define RELAY2_B 17
#define RELAY3_A 18
#define RELAY3_B 19
#define RELAY4_A 20
#define RELAY4_B 21

#define SWITCH1 26
#define SWITCH2 25
#define SWITCH3 24
#define SWITCH4 12
#define SWITCH5 11

#define MIDI_TX 1;
#define MIDI_RX 0;

#define LED1 5
#define LED2 4
#define LED3 3
#define LED4 2

#define LEDR 6
#define LEDG 7
#define LEDB 8

#define MODE_READ 0
#define MODE_WRITE 1
#define MODE_WRITEMIDI 2

/* These are definitions for the MIDI CC values
 * You'll have to manually assign BYPASS to each block, 
 but the state can be controlled better this way by 
 sending 0 for bypass, and 127 for enabled
 */

#define MIDI_CC_A 11
#define MIDI_CC_B 12
#define MIDI_CC_C 13
#define MIDI_CC_D 14
///#define MIDI_HXSTOMP_BLOCK5 15
//#define MIDI_HXSTOMP_BLOCK6 16

//#define DEBUG //comment out this line to hide serial print messages.

struct Preset
{
  bool loop_a;
  bool loop_b;
  bool loop_c;
  bool loop_d;

  bool midi_cc_a;
  bool midi_cc_b;
  bool midi_cc_c;
  bool midi_cc_d;
};

int switcherMode = 0; //start in manual mode?

int activePreset = 0;

//temporary values for these variables
bool loopAEnabled = false;
bool loopBEnabled = false;
bool loopCEnabled = false;
bool loopDEnabled = false;

bool midi_a_enabled = false;
bool midi_b_enabled = false;
bool midi_c_enabled = false;
bool midi_d_enabled = false;

bool midi_a_last = false;
bool midi_b_last = false;
bool midi_c_last = false;
bool midi_d_last = false;

struct Preset preset[4]; //array of presets

unsigned long lastDebounceTimeA = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTimeB = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTimeC = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTimeD = 0;  // the last time the output pin was toggled
unsigned long lastDebounceTimeMode = 0;  // the last time the output pin was toggled

unsigned long debounceDelay = 200;    // the debounce time; increase if the output flickers

//blinking must be performed asynchronously to avoid delay in switching
//here we can declare time of each blink
unsigned long led_red_blink_time = 250; //ms
unsigned long led_blue_blink_time = 250; //ms

//we need this variable to store last time of blink in milliseconds
unsigned long led_red_last_blink;
unsigned long led_blue_last_blink;

//we need this to keep track of and send the brightness value 
int led_red_brightness = 256;
int led_blue_brightness = 256;

int pulseCounter = 1;

void setup() {
  // put your setup code here, to run once:
  
  //Serial.begin(9600);
  
  MIDI.begin(MIDI_CHANNEL_OMNI);
  
  pinMode(RELAY1_A, OUTPUT);
  pinMode(RELAY1_B, OUTPUT);
  pinMode(RELAY2_A, OUTPUT);
  pinMode(RELAY2_B, OUTPUT);
  pinMode(RELAY3_A, OUTPUT);
  pinMode(RELAY3_B, OUTPUT);
  pinMode(RELAY4_A, OUTPUT);
  pinMode(RELAY4_B, OUTPUT);

  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);

  pinMode(SWITCH1, INPUT_PULLUP);
  pinMode(SWITCH2, INPUT_PULLUP);
  pinMode(SWITCH3, INPUT_PULLUP);
  pinMode(SWITCH4, INPUT_PULLUP);
  pinMode(SWITCH5, INPUT_PULLUP);
  
  // OUTPUT LEDs
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);
  pinMode(LED4, OUTPUT);
  
  pinMode(LEDR, OUTPUT);
  pinMode(LEDG, OUTPUT);
  pinMode(LEDB, OUTPUT);
}

void setColorLED(int r,int g,int b){
  analogWrite(LEDR, 256 - r); //Red
  analogWrite(LEDG, 256 - g); //Green
  analogWrite(LEDB, 256 - b); //Blue
}

void loop() {
  // put your main code here, to run repeatedly:
  readModeSwitch();
  
  readFootSwitches();

  if (switcherMode == MODE_WRITE) {
    //blink the mode LED red
    if((millis() - led_red_last_blink) >= led_red_blink_time){
      if(led_red_brightness == 256){
      led_red_brightness = 5;
      }else{
      led_red_brightness = 256;
      }
      led_red_last_blink = millis();
    }
    setColorLED(led_red_brightness, 0, 0); // red indicates manual/write mode

    //read the current preset into the edit buffer
    preset[activePreset].loop_a = loopAEnabled;
    preset[activePreset].loop_b = loopBEnabled;
    preset[activePreset].loop_c = loopCEnabled;
    preset[activePreset].loop_d = loopDEnabled;
    
    //set LEDs to indicate the current loop states for the current preset
    setLEDStates(loopAEnabled, loopBEnabled, loopCEnabled, loopDEnabled);
    
    //set the relay states
    setRelayStates(loopAEnabled, loopBEnabled, loopCEnabled, loopDEnabled);
    
  } else if(switcherMode == MODE_READ){
    
    setColorLED(0, 256, 0); //green indicates preset/read mode
    
    switch(activePreset){ //set LED to display the current preset
      case 0:
        analogWrite(LED1, 256);
        analogWrite(LED2, 0);
        analogWrite(LED3, 0);
        analogWrite(LED4, 0);
      break;
      case 1:
        analogWrite(LED1, 0);
        analogWrite(LED2, 256);
        analogWrite(LED3, 0);
        analogWrite(LED4, 0);
      break;
      case 2:
        analogWrite(LED1, 0);
        analogWrite(LED2, 0);
        analogWrite(LED3, 256);
        analogWrite(LED4, 0);
      break;
      case 3:
        analogWrite(LED1, 0);
        analogWrite(LED2, 0);
        analogWrite(LED3, 0);
        analogWrite(LED4, 256);
      break;
    }

    //set placeholder variables 
    loopAEnabled = preset[activePreset].loop_a;
    loopBEnabled = preset[activePreset].loop_b;
    loopCEnabled = preset[activePreset].loop_c;
    loopDEnabled = preset[activePreset].loop_d;

    midi_a_enabled = preset[activePreset].midi_cc_a;
    midi_b_enabled = preset[activePreset].midi_cc_b;
    midi_c_enabled = preset[activePreset].midi_cc_c;
    midi_d_enabled = preset[activePreset].midi_cc_d;

    //set the relays according to the current presets loop states
    setRelayStates(loopAEnabled, loopBEnabled, loopCEnabled, loopDEnabled);

    //TODO: Serial write the MIDI CC changes
    sendMIDI_CCs(midi_a_enabled, midi_b_enabled, midi_c_enabled, midi_d_enabled); 
    
  } else if (switcherMode == MODE_WRITEMIDI) {
    //blink the mode LED red
    if((millis() - led_blue_last_blink) >= led_blue_blink_time){
      if(led_blue_brightness == 256){
      led_blue_brightness = 5;
      }else{
      led_blue_brightness = 256;
      }
      led_blue_last_blink = millis();
    }
    setColorLED(0, 0, led_blue_brightness); // red indicates manual/write mode
    
    //read the current preset into the edit buffer
    preset[activePreset].midi_cc_a = midi_a_enabled;
    preset[activePreset].midi_cc_b = midi_b_enabled;
    preset[activePreset].midi_cc_c = midi_c_enabled;
    preset[activePreset].midi_cc_d = midi_d_enabled;
    
    //set LEDs to indicate the current loop states for the current preset
    setLEDStates(midi_a_enabled, midi_b_enabled, midi_c_enabled, midi_d_enabled);

    //TODO: Serial write the MIDI CC changes
    sendMIDI_CCs(midi_a_enabled, midi_b_enabled, midi_c_enabled, midi_d_enabled);
  }
}

void readModeSwitch(){
  // Read the footswitches
  int modeSwitchState = digitalRead(SWITCH1);
  //Mode switch must be done regardless of current mode
  if ( (millis() - lastDebounceTimeMode) > debounceDelay) {
      if (modeSwitchState == LOW) {
        if (switcherMode == 0){
          switcherMode = 1;
          //Serial.print("Mode changed to ");
          //Serial.print("WRITE ANALOG\n");
        } else if (switcherMode == 1){
          switcherMode = 2;
          //Serial.print("Mode changed to ");
          //Serial.print("WRITE MIDI\n");
        } else {
          switcherMode = 0;
          //Serial.print("Mode changed to ");
          //Serial.print("READ\n");
          printPresetState();
        }
      lastDebounceTimeMode = millis();
    }
  }
}

void readFootSwitches(){
  int switchAState = digitalRead(SWITCH2);
  int switchBState = digitalRead(SWITCH3);
  int switchCState = digitalRead(SWITCH4);
  int switchDState = digitalRead(SWITCH5);

  switch (switcherMode){
  case MODE_WRITE:
    if ( (millis() - lastDebounceTimeA) > debounceDelay) {
      if (switchAState == LOW && loopAEnabled == false) {
        loopAEnabled = true;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop A Enabled\n");
        lastDebounceTimeA = millis();
      } else if(switchAState == LOW && loopAEnabled == true)  {
        loopAEnabled = false;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop A Bypassed\n");
        lastDebounceTimeA = millis();
      }
    }
    
    if ( (millis() - lastDebounceTimeB) > debounceDelay) {
      if (switchBState == LOW && loopBEnabled == false) {
        loopBEnabled = true;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop B Enabled\n");
        lastDebounceTimeB = millis();
      } else if(switchBState == LOW && loopBEnabled == true){
        loopBEnabled = false;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop B Bypassed\n");
        lastDebounceTimeB = millis();
      }
    }
    
    if ( (millis() - lastDebounceTimeC) > debounceDelay) {
      if (switchCState == LOW && loopCEnabled == false) {
        loopCEnabled = true;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop C Enabled\n");
        lastDebounceTimeC = millis();
      } else if(switchCState == LOW && loopCEnabled == true){
        loopCEnabled = false;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop C Bypassed\n");
        lastDebounceTimeC = millis();
      }
    }
    
    if ( (millis() - lastDebounceTimeD) > debounceDelay) {
      if (switchDState == LOW && loopDEnabled == false) {
        loopDEnabled = true;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop D Enabled\n");
        lastDebounceTimeD = millis();
      } else if(switchDState == LOW && loopDEnabled == true){
        loopDEnabled = false;
        //Serial.print("Preset: ");
        //Serial.print(activePreset);
        //Serial.print(" Loop D Bypassed\n");
        lastDebounceTimeD = millis();
      }
    }
  break;
  case MODE_READ:
    if ( (millis() - lastDebounceTimeA) > debounceDelay) {
      if (switchAState == LOW) {
        activePreset = 0;
        //Serial.print("Preset 0 Selected\n");
        printPresetState();
        lastDebounceTimeA = millis();
      } 
    }
    if ( (millis() - lastDebounceTimeB) > debounceDelay) {
      if (switchBState == LOW) {
        activePreset = 1;
        //Serial.print("Preset 1 Selected\n");
        printPresetState();
        lastDebounceTimeB = millis();
      } 
    }
    if ( (millis() - lastDebounceTimeC) > debounceDelay) {
      if (switchCState == LOW) {
        activePreset = 2;
        //Serial.print("Preset 2 Selected\n");
        printPresetState();
        lastDebounceTimeC = millis();
      } 
    }
    if ( (millis() - lastDebounceTimeD) > debounceDelay) {
      if (switchDState == LOW) {
        activePreset = 3;
        //Serial.print("Preset 3 Selected\n");
        printPresetState();
        lastDebounceTimeD = millis();
      } 
    }
  break;
  case MODE_WRITEMIDI:
    if ( (millis() - lastDebounceTimeA) > debounceDelay) {
      if (switchAState == LOW && midi_a_enabled == false) {
          midi_a_enabled = true;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 11 Enabled\n");
          lastDebounceTimeA = millis();
        } else if(switchAState == LOW && midi_a_enabled == true)  {
          midi_a_enabled = false;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 11 Bypassed\n");
          lastDebounceTimeA = millis();
        }
    }
    if ( (millis() - lastDebounceTimeB) > debounceDelay) {
      if (switchBState == LOW && midi_b_enabled == false) {
          midi_b_enabled = true;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 12 Enabled\n");
          lastDebounceTimeB = millis();
        } else if(switchBState == LOW && midi_b_enabled == true)  {
          midi_b_enabled = false;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 12 Bypassed\n");
          lastDebounceTimeB = millis();
        }
    }
    if ( (millis() - lastDebounceTimeC) > debounceDelay) {
      if (switchCState == LOW && midi_c_enabled == false) {
          midi_c_enabled = true;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 13 Enabled\n");
          lastDebounceTimeC = millis();
        } else if(switchCState == LOW && midi_c_enabled == true)  {
          midi_c_enabled = false;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 13 Bypassed\n");
          lastDebounceTimeC = millis();
        }
    }
    if ( (millis() - lastDebounceTimeD) > debounceDelay) {
      if (switchDState == LOW && midi_d_enabled == false) {
          midi_d_enabled = true;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 14 Enabled\n");
          lastDebounceTimeD = millis();
        } else if(switchDState == LOW && midi_d_enabled == true)  {
          midi_d_enabled = false;
          //Serial.print("Preset: ");
          //Serial.print(activePreset);
          //Serial.print(" MIDI CC 14 Bypassed\n");
          lastDebounceTimeD = millis();
        }
    }
  }
}

void setLEDStates(bool a, bool b, bool c, bool d)
{
  if (a == true) {
      analogWrite(LED1, 256);
    } else {
      analogWrite(LED1, 1);
    }

    if (b == true ) {
      analogWrite(LED2, 256);
    } else {
      analogWrite(LED2, 1);
    }

    if (c == true) {
      analogWrite(LED3, 256);
    } else {
      analogWrite(LED3, 1);
    }

    if (d == true) {
      analogWrite(LED4, 256);
    } else {
      analogWrite(LED4, 1);
    }
}

void printPresetState(){
  #ifdef DEBUG
  //Serial.print(activePreset);
  //Serial.print("\n");
  //Serial.print(preset[activePreset].loop_a);
  //Serial.print(preset[activePreset].loop_b);
  //Serial.print(preset[activePreset].loop_c);
  //Serial.print(preset[activePreset].loop_d);
  //Serial.print(" ");
  //Serial.print(preset[activePreset].midi_cc_a);
  //Serial.print(preset[activePreset].midi_cc_b);
  //Serial.print(preset[activePreset].midi_cc_c);
  //Serial.print(preset[activePreset].midi_cc_d);
  //Serial.print("\n");
  #endif
}

void setRelayStates(bool a, bool b, bool c, bool d)
{  
  if (a == 1) {
    digitalWrite(RELAY1_A, HIGH);
    digitalWrite(RELAY1_B, LOW);
  } else {
    digitalWrite(RELAY1_A, LOW);
    digitalWrite(RELAY1_B, HIGH);
  }

  if (b == 1) {
    digitalWrite(RELAY2_A, HIGH);
    digitalWrite(RELAY2_B, LOW);
  } else {
    digitalWrite(RELAY2_A, LOW);
    digitalWrite(RELAY2_B, HIGH);
  }

  if (c == 1) {
    digitalWrite(RELAY3_A, HIGH);
    digitalWrite(RELAY3_B, LOW);
  } else {
    digitalWrite(RELAY3_A, LOW);
    digitalWrite(RELAY3_B, HIGH);
  }

  if (d == 1) {
    digitalWrite(RELAY4_A, HIGH);
    digitalWrite(RELAY4_B, LOW);
  } else {
    digitalWrite(RELAY4_A, LOW);
    digitalWrite(RELAY4_B, HIGH);
  }
  //delay(500);
}

void sendMIDI_CCs(bool a, bool b, bool c, bool d){
  if (a == 1 && a != midi_a_last) {
    MIDI.sendControlChange(MIDI_CC_A, 127, 1);
    //Serial.print("MIDI A 127 \n");
  } else if (a != midi_a_last){
    MIDI.sendControlChange(MIDI_CC_A, 0, 1);
    //Serial.print("MIDI A 0 \n");
  }

  if (b == 1 && b != midi_b_last) {
    MIDI.sendControlChange(MIDI_CC_B, 127, 1);
    //Serial.print("MIDI B 127 \n");
  } else if (b != midi_b_last){
    MIDI.sendControlChange(MIDI_CC_B, 0, 1);
    //Serial.print("MIDI B 0 \n");
  }

  if (c == 1 && c != midi_c_last) {
    MIDI.sendControlChange(MIDI_CC_C, 127, 1);
    //Serial.print("MIDI C 127 \n");
  } else if (c != midi_c_last){
    MIDI.sendControlChange(MIDI_CC_C, 0, 1);
    //Serial.print("MIDI C 0 \n");
  }

  if (d == 1 && d != midi_d_last) {
    MIDI.sendControlChange(MIDI_CC_D, 127, 1);
    //Serial.print("MIDI D 127 \n");
  } else if (d != midi_d_last){
    MIDI.sendControlChange(MIDI_CC_D, 0, 1);
    //Serial.print("MIDI D 0 \n");
  }
  
  midi_a_last = a;
  midi_b_last = b;
  midi_c_last = c;
  midi_d_last = d;
}
