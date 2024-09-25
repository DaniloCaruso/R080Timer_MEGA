#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <ArduinoSTL.h>
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
  // Serial.println("MODE BUTTON CLICKED");
  // Serial.println("Current Modality before change: " + (String)operationStatus.current.modalityTypeStatus.current.value);
  
  operationStatus.current.switchModality(&operationStatus.current.modalityTypeStatus);

  // Serial.println("Current Modality after change: " + (String)operationStatus.current.modalityTypeStatus.current.value);
}

void onPrecisionButtonEvent(EncoderButton& encoderButton) {
  // Serial.println("PRECISION BUTTON CLICKED");
  // Serial.println("Current Precision before change: " + (String)operationStatus.current.precisionTypeStatus.current.value);
  
  operationStatus.current.switchPrecisionType(&operationStatus.current.precisionTypeStatus);

  // Serial.println("Current Precision after change: " + (String)operationStatus.current.precisionTypeStatus.current.value);
}


void registerClickHandler() {
  encoderModeButton.setClickHandler(onModeButtonEvent);
  encoderPrecisionButton.setClickHandler(onPrecisionButtonEvent);
}

void evaluateButtons() {
  encoderModeButton.update();
  encoderPrecisionButton.update();
}

void evaluateEncoderMod() {
  long newEncoderPosition = encoderMode.read() >> 2;
  int timerModeIndex = (newEncoderPosition % 3 + 3) % 3;
  // Serial.println("ENCODER MODE INDEX: " + (String)timerModeIndex);
  setOperationStatus(timerModeIndex);
  showModalityType();
  
}

void evaluateEncoderPrec() {
  long newEncoderPosition = encoderPrecision.read() >> 2;
  int precisionIndex = (newEncoderPosition % 11 + 11) % 11;
  // Serial.println("ENCODER PREC INDEX: " + (String)precisionIndex);
  setOperationPrecision(precisionIndex);
  showPrecision();
}

#endif // ENCODER_H
