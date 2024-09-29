// RUNNING_MGMT_H
#ifndef RUNNING_MGMT_H
#define RUNNING_MGMT_H

#include <Arduino.h>
#include "variables.h"
#include "functions/keypad_mgmt.h"
#include "functions/encoder_mgmt.h"
#include "functions/timer_manager.h"

// PROTOTYPES
void onStop();
void manageStarted();
void configureMainButton();
void onMainButtonClick();
void manageMainButtonAction();

// IMPLEMENTATIONS

void configureMainButton() {
  mainButton.attachClick(onMainButtonClick);
  mainButton.setDebounceMs(DEBOUNCE_TIME);
  mainButton.setClickMs(100);
  mainButton.setPressMs(1000);
}

void onMainButtonClick() {
  isMainBtnPressed = !isMainBtnPressed;
}

void onStop() {
  stopAllTimers();
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
