
// RUNNING_MGMT_H
#ifndef RUNNING_MGMT_H
#define RUNNING_MGMT_H

#include <Arduino.h>
#include "functions/base.h"
#include "variables.h"

// PROTOTYPES
void setMainButtonPressed(boolean isPressed);
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

void setMainButtonPressed(boolean isPressed) {
  isMainBtnPressed = isPressed;
}

void setActionStarted(boolean isStarted) {
  isActionStarted = isStarted;
}

void evaluateMainBtnStatus() {
  mainBtnTemporalStatus["current"] = digitalRead(MAIN_BTN_PIN);
  if (mainBtnTemporalStatus["current"] != mainBtnTemporalStatus["last"]) {
    setMainButtonPressed(pushOrPullActMode == mainBtnTemporalStatus["current"]); 
    setLastMainBtnStatus(mainBtnTemporalStatus["current"]);
  }
  setErrlet(); 
}

void evaluateMainButtonActionStatus() {
  mainBtnActionStatus["TO_STOP"] = isActionStarted && isMainBtnPressed;
  mainBtnActionStatus["TO_START"] = !isActionStarted && isMainBtnPressed;
  mainBtnActionStatus["STARTED"] = isActionStarted && !isMainBtnPressed;
  mainBtnActionStatus["STOPPED"] = !isActionStarted && !isMainBtnPressed;
}

void manageStopped() {

}

void manageToStop() {
  channelsRelayActivated[channel] ? digitalWrite(RELAY_PIN, HIGH) : noop();
  setActionStarted(false);
  setMainButtonPressed(false);
}

void manageStarted() {

}

void manageToStart() {

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
    writeRow(1, " RESET W KEY D ");
  }
}



#endif // RUNNING_MGMT_H