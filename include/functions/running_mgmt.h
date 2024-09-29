
// RUNNING_MGMT_H
#ifndef RUNNING_MGMT_H
#define RUNNING_MGMT_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "variables.h"
#include "functions/keypad_mgmt.h"
#include "functions/encoder_mgmt.h"


// PROTOTYPES
void setLastMainBtnStatus(int lastValue);
void evaluateMainBtnStatus();
void evaluateMainButtonActionStatus();
void onStop();
void manageToStart();
void manageToStop();
void manageStarted();

// IMPLEMENTATIONS

const unsigned long BUTTON_COOLDOWN = 50; // Tempo minimo tra le letture del pulsante in millisecondi
unsigned long lastButtonEventTime = 0;

void mainButtonHandler() {
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > DEBOUNCE_TIME) {
    isMainBtnPressed = !isMainBtnPressed;
    Serial.println("PRESSED: "+ (String)isMainBtnPressed);
    lastInterruptTime = interruptTime;
  }
}


void onStop() {
  relayStart(false);
  isActionStarted = false;
  evaluateEncoderMod();
  evaluateEncoderPrec();
  evaluateButtons();
  evaluateKey();
}

void manageStarted() {
  operationStatus.current.startedAction();
}

void manageMainButtonAction() {
  !isMainBtnPressed ? onStop() : manageStarted();
}



#endif // RUNNING_MGMT_H