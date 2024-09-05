#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include <EncoderButton.h>
#include "variables.h"
#include "functions/operations.h"

// PROTOTYPES

void onModeButtonEvent(EncoderButton& encoderButton);
void onPrecisionButtonEvent(EncoderButton& encoderButton);
void registerClickHandler();


// IMPLEMENTATIONS

void onModeButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchModality(&operationStatus.current.modalityType);
}

void onPrecisionButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchPrecision(&operationStatus.current.precisionType);
}

void registerClickHandler() {
  encoderModeButton.setClickHandler(onModeButtonEvent);
  encoderPrecisionButton.setClickHandler(onPrecisionButtonEvent);
}

void evaluateEncoderMod() {
  long newEncoderPosition = encoderMode.read() >> 2;
  int timerModeIndex = newEncoderPosition % 3 + 1;
  setOperationStatus(timerModeIndex);
}

#endif // ENCODER_H
