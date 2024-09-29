#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>
#include <EncoderButton.h>
#include "variables.h"
#include "functions/operations.h"

// PROTOTYPES
void onModeButtonEvent(EncoderButton& encoderButton);
void onPrecisionButtonEvent(EncoderButton& encoderButton);
void registerClickHandler();
void evaluateButtons();
void evaluateEncoderMod();
void evaluateEncoderPrec();

// IMPLEMENTATIONS

void onModeButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchModality(&operationStatus.current.modalityTypeStatus);
}

void onPrecisionButtonEvent(EncoderButton& encoderButton) {
  // Quando si cambia tipo precisione (SECONDS <-> FSTOP)
  // inizializza F-stop state col tempo corrente come base
  operationStatus.current.switchPrecisionType(&operationStatus.current.precisionTypeStatus);
  if (operationStatus.current.precisionTypeStatus.current.id == FSTOP) {
    fstopState.setBase(timeSet > 0 ? timeSet : 1);
  }
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
  setOperationStatus(timerModeIndex);
  showModalityType();
}

void evaluateEncoderPrec() {
  long newEncoderPosition = encoderPrecision.read() >> 2;
  int precisionIndex = (newEncoderPosition % PRECISION_COUNT + PRECISION_COUNT) % PRECISION_COUNT;
  setOperationPrecision(precisionIndex);
  showPrecision();

  // Se in modalità F-stop, ricalcola il tempo col nuovo valore di precisione
  if (operationStatus.current.precisionTypeStatus.current.id == FSTOP && fstopState.baseTime > 0) {
    int precisionValue = operationStatus.current.precisionStatus.current.value;
    timeSet = fstopState.getCalculatedTime(precisionValue);
    appoTimeSet = timeSet;
    channelTime[channel - 1] = timeSet;
    showTime(timeSet);
  }
}

#endif // ENCODER_H
