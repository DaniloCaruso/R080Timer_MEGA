#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "types.h"
#include "constants.h"
#include "variables.h"
#include "functions/presentation.h"
#include "functions/base.h"
#include "functions/actions.h"

// PROTOTYPES
void switchModality(ModalityType* type);
void switchPrecision(PrecisionType* type);
void initializeOperations();
Operation getOperationByName(String name);
boolean currentOpDiffsFromLast();
boolean isLastOpEmpty();
boolean isCurrentOpEmpty();
void setOperationStatus(int currentIndex);

// IMPLEMENTATIONS
void noAction() { /* Implementazione */ } // TEMPORANEA

void switchModality(ModalityType* type) {
    *type = (type->id == Modality::CONTINOUS) ? 
      ModalityType(Modality::STEP_BY_STEP, "PP") : 
      ModalityType(Modality::CONTINOUS, "CO");
}

void switchPrecision(PrecisionType* type) {
    *type = (type->id == Precision::SECONDS) ? 
      PrecisionType(Precision::FSTOP, "F") : 
      PrecisionType(Precision::SECONDS, "S");
}

void initializeOperations() {

    operations[FOCUS] = Operation(
      MODES_TEXTS[FOCUS], 
      showFocus, 
      operationActions[FOCUS].toStartAction,
      operationActions[FOCUS].startedAction
    );

    operations[COUNTDOWN] = Operation(
      MODES_TEXTS[COUNTDOWN], 
      PrecisionType(Precision::SECONDS, "S"), 
      showCountdown, 
      switchPrecision, 
      operationActions[COUNTDOWN].toStartAction,
      operationActions[COUNTDOWN].startedAction
    );
    
    operations[TEST_STRIP] = Operation(
      MODES_TEXTS[TEST_STRIP], 
      ModalityType(Modality::CONTINOUS, "CO"), 
      PrecisionType(Precision::SECONDS, "S"), 
      showTestStrip, 
      switchModality, 
      switchPrecision, 
      operationActions[TEST_STRIP].toStartAction,
      operationActions[TEST_STRIP].startedAction
    );

    setOperationStatus(0);
}

Operation getOperationByName(String name) {
  int arrayLength = sizeof(operations)/sizeof(operations[0]);
  for (int i=0; i < arrayLength; i++) {

    if (operations[i].name == name) {
      return operations[i];
      break;
    }
  }
  return Operation();
}

boolean currentOpDiffsFromLast() {
  return operationStatus.current.name != operationStatus.last.name;
}

boolean isLastOpEmpty() {
  return operationStatus.last.name == "";
}

boolean isCurrentOpEmpty() {
  return operationStatus.current.name == "";
}

void setOperationStatus(int currentIndex) {

  operationStatus.last = operationStatus.current;
  operationStatus.current = operations[currentIndex];
}

#endif // OPERATIONS_H
