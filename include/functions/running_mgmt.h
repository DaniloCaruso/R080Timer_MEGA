
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
void manageStopped();
void manageToStart();
void manageToStop();
void manageStarted();

// IMPLEMENTATIONS
void setLastMainBtnStatus(int lastValue) {
  mainBtnTemporalStatus["last"] = lastValue;
}

void evaluateMainBtnStatus() {
  mainBtnTemporalStatus["current"] = digitalRead(MAIN_BTN_PIN);
  // Serial.println("MAIN BUTTON STATUS CURRENT"+ (String) mainBtnTemporalStatus["current"]);
  if (mainBtnTemporalStatus["current"] != mainBtnTemporalStatus["last"]) {
    isMainBtnPressed = pushOrPullActMode == mainBtnTemporalStatus["current"]; 
    setLastMainBtnStatus(mainBtnTemporalStatus["current"]);
  }
  setErrlet(); 
}

void evaluateMainButtonActionStatus() {
  mainBtnActionStatus["TO_STOP"] = isActionStarted && isMainBtnPressed;
  mainBtnActionStatus["TO_START"] = !isActionStarted && isMainBtnPressed;
  mainBtnActionStatus["STARTED"] = isActionStarted && !isMainBtnPressed;
  mainBtnActionStatus["STOPPED"] = !isActionStarted && !isMainBtnPressed;

  // Serial.println("TO_STOP = " + (String)mainBtnActionStatus["TO_STOP"]);
  // Serial.println("TO_START = " + (String)mainBtnActionStatus["TO_START"]);
  // Serial.println("STARTED = " + (String)mainBtnActionStatus["STARTED"]);
  // Serial.println("STOPPED = " + (String)mainBtnActionStatus["STOPPED"]);
}

void manageStopped() {
  evaluateEncoderMod();
  evaluateEncoderPrec();
  evaluateButtons();
  evaluateKey();
}

void manageToStop() {
  releStartStop(false);
  isActionStarted = false;
  isMainBtnPressed = false;
}

void manageStarted() {
  operationStatus.current.startedAction();
}

void manageToStart() {
  operationStatus.current.toStartAction();
}

void manageMainButtonAction() {
  evaluateMainButtonActionStatus();
   
  if (mainBtnActionStatus["STOPPED"]) {
    manageStopped();
     
  } else if (mainBtnActionStatus["TO_START"]) {
    manageToStart();
  } else if (mainBtnActionStatus["STARTED"]) {
    manageStarted();
  } else if (mainBtnActionStatus["TO_STOP"]) {
    manageToStop();
  } else {
    writeRow(0, "    ERROR!     ");
    writeRow(1, " RESET W/KEY D ");
  }
}



#endif // RUNNING_MGMT_H