#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "variables.h"
#include "functions/actions.h"

// PROTOTYPES
void switchModality(ModalityTypeStruct* type);
void switchPrecisionType(PrecisionTypeStruct* type);
void initializeOperations();
Operation getOperationByName(String name);
boolean currentOpDiffsFromLast();
boolean isLastOpEmpty();
boolean isCurrentOpEmpty();
void setOperationStatus(int currentIndex);

// IMPLEMENTATIONS
void noAction() { /* Implementazione */ } // TEMPORANEA

void switchModality(ModalityTypeStruct* type) {
    *type = (type->id == ModalityType::CONTINOUS) ? 
      ModalityTypeStruct(ModalityType::STEP_BY_STEP, "PP") : 
      ModalityTypeStruct(ModalityType::CONTINOUS, "CO");
}

void switchPrecisionType(PrecisionTypeStruct* type) {
    *type = (type->id == PrecisionType::SECONDS) ? 
      PrecisionTypeStruct(PrecisionType::FSTOP, "F") : 
      PrecisionTypeStruct(PrecisionType::SECONDS, "S");
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
      PrecisionTypeStruct(PrecisionType::SECONDS, "S"), 
      showCountdown, 
      switchPrecisionType, 
      operationActions[COUNTDOWN].toStartAction,
      operationActions[COUNTDOWN].startedAction
    );
    
    operations[TEST_STRIP] = Operation(
      MODES_TEXTS[TEST_STRIP], 
      ModalityTypeStruct(ModalityType::CONTINOUS, "CO"), 
      PrecisionTypeStruct(PrecisionType::SECONDS, "S"), 
      showTestStrip, 
      switchModality, 
      switchPrecisionType, 
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

void setOperationPrecision(int precisionIndex) {

  operationStatus.current.setPrecision(precisionIndex);
}

#endif // OPERATIONS_H
