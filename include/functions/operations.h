#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <Arduino.h>
#include "variables.h"
#include "functions/actions.h"

// PROTOTYPES
void switchModality(ModalityTypeStatus* type);
void switchPrecisionType(PrecisionTypeStatus* type);
void initializeOperations();
Operation getOperationById(uint8_t id);
void setOperationStatus(int currentIndex);
void setOperationPrecision(int precisionIndex);

// IMPLEMENTATIONS

void switchModality(ModalityTypeStatus* type) {
    ModalityTypeStruct newModalityType = (operationStatus.current.modalityTypeStatus.current.id == ModalityType::CONTINOUS) ?
    ModalityTypeStruct(ModalityType::STEP_BY_STEP, "PP") :
    ModalityTypeStruct(ModalityType::CONTINOUS, "CO");

    operationStatus.current.modalityTypeStatus.last = operationStatus.current.modalityTypeStatus.current;
    operationStatus.current.modalityTypeStatus.current = newModalityType;
    operationStatus.current.showPresentation();
}

void switchPrecisionType(PrecisionTypeStatus* type) {
    PrecisionTypeStruct newPrecisionType = (operationStatus.current.precisionTypeStatus.current.id == PrecisionType::SECONDS) ?
        PrecisionTypeStruct(PrecisionType::FSTOP, "F") :
        PrecisionTypeStruct(PrecisionType::SECONDS, "S");

    operationStatus.current.precisionTypeStatus.last = operationStatus.current.precisionTypeStatus.current;
    operationStatus.current.precisionTypeStatus.current = newPrecisionType;
    operationStatus.current.showPresentation();
}

void initializeOperations() {
    // MODES_TEXT_N sono stringhe PROGMEM (constants.h); il puntatore viene
    // conservato in Operation::name ma non e' mai dereferenziato direttamente:
    // per rendering usiamo showModeName(modeId) + strcpy_P.
    operations[FOCUS] = Operation(
      MODES_TEXT_0,
      showFocus,
      focus
    );

    operations[COUNTDOWN] = Operation(
      MODES_TEXT_1,
      PrecisionTypeStruct(PrecisionType::SECONDS, "S"),
      showCountdown,
      switchPrecisionType,
      countdown
    );

    operations[TEST_STRIP] = Operation(
      MODES_TEXT_2,
      ModalityTypeStruct(ModalityType::CONTINOUS, "CO"),
      PrecisionTypeStruct(PrecisionType::SECONDS, "S"),
      showTestStrip,
      switchModality,
      switchPrecisionType,
      testStrip
    );

    setOperationStatus(0);
}

Operation getOperationById(uint8_t id) {
  if (id >= 3) return Operation();
  return operations[id];
}

void setOperationStatus(int currentIndex) {
  if (operations[currentIndex].name != operationStatus.current.name) {
   operationStatus.last = operationStatus.current;
   operationStatus.current = operations[currentIndex];
   operationStatus.current.showPresentation();
   resetTime();
   resetTimerState();
  }
}

void setOperationPrecision(int precisionIndex) {
  operationStatus.current.setPrecisionStatus(precisionIndex);
}

#endif // OPERATIONS_H
