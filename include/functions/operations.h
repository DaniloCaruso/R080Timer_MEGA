#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "types.h"
#include "constants.h"
#include "variables.h"
#include "functions/presentation.h"
#include "functions/base.h"

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

    operations[0] = Operation(
      MODES_TEXTS[0], 
      showFocus, 
      noAction
    );

    operations[1] = Operation(
      MODES_TEXTS[1], 
      PrecisionType(Precision::SECONDS, "S"), 
      showCountdown, 
      switchPrecision, 
      noAction
    );
    
    operations[2] = Operation(
      MODES_TEXTS[2], 
      ModalityType(Modality::CONTINOUS, "CO"), 
      PrecisionType(Precision::SECONDS, "S"), 
      showFocus, 
      switchModality, 
      switchPrecision, 
      noAction
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
