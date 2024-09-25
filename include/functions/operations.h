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

void switchModality(ModalityTypeStatus* type) {
    // Serial.println("Switching Modality...");
    // Serial.println("Current Modality Type: " + (String)operationStatus.current.modalityTypeStatus.current.value);

    // Alterna tra CONTINUOUS e STEP_BY_STEP
    ModalityTypeStruct newModalityType = (operationStatus.current.modalityTypeStatus.current.id == ModalityType::CONTINOUS) ? 
    ModalityTypeStruct(ModalityType::STEP_BY_STEP, "PP") : 
    ModalityTypeStruct(ModalityType::CONTINOUS, "CO");

    operationStatus.current.modalityTypeStatus.last = operationStatus.current.modalityTypeStatus.current;
    operationStatus.current.modalityTypeStatus.current = newModalityType;

    // Serial.println("New Modality Type: " + (String)operationStatus.current.modalityTypeStatus.current.value);
    operationStatus.current.showPresentation();
}

void switchPrecisionType(PrecisionTypeStatus* type) {
    // Serial.println("Switching Precision Type...");
    // Serial.println("Current Precision Type: " + (String)operationStatus.current.precisionTypeStatus.current.value);

    // Alterna tra SECONDS e FSTOP
    PrecisionTypeStruct newPrecisionType = (operationStatus.current.precisionTypeStatus.current.id == PrecisionType::SECONDS) ? 
        PrecisionTypeStruct(PrecisionType::FSTOP, "F") : 
        PrecisionTypeStruct(PrecisionType::SECONDS, "S");

    operationStatus.current.precisionTypeStatus.last = operationStatus.current.precisionTypeStatus.current;
    operationStatus.current.precisionTypeStatus.current = newPrecisionType;

    // Serial.println("New Precision Type: " + (String)operationStatus.current.precisionTypeStatus.current.value);
    operationStatus.current.showPresentation();
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


    // Serial.println("[SETUP 12] DEBUG ###########################################################################################");
    // for (size_t i = 0; i < 3; i++) {
    //   Serial.println("---- OPERATION["+(String)i+"]");
    //   Serial.println("-------- NAME: " + operations[i].name);
    //   Serial.println("-------- PRECISION TYPE: " + (String)operations[i].precisionTypeStatus.current.id + " -> ["+(String)operations[i].precisionTypeStatus.current.value+"]");
    //   Serial.println("-------- PRECISION: " + (String)operations[i].precisionStatus.current.value + " -> ["+operations[i].precisionStatus.current.string+"]");
    //   Serial.println("-------- MODALITY TYPE: " + (String)operations[i].modalityTypeStatus.current.id + " -> ["+(String)operations[i].modalityTypeStatus.current.value+"]");
    // }
    // Serial.println("###########################################################################################################");
    

    setOperationStatus(0);
//     Serial.println("[SETUP 12] DEBUG AFTER SET OPERATION STATUS 0");

//     Serial.println("---------- OPERATION STATUS CURRENT, NAME: " + operationStatus.current.name );

//      Serial.println("---------- OPERATION STATUS LAST, NAME: " + operationStatus.last.name );
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
  if (operations[currentIndex].name != operationStatus.current.name) {
  //  Serial.println("Setting operation status to index: " + (String)currentIndex);
  //  Serial.println("Previous current precision: " + (String)operationStatus.current.precisionTypeStatus.current.value);
   
   operationStatus.last = operationStatus.current;
   operationStatus.current = operations[currentIndex];

  //  Serial.println("New current precision: " + (String)operationStatus.current.precisionTypeStatus.current.value);
   operationStatus.current.showPresentation();
   resetTime();
  }
}

void setOperationPrecision(int precisionIndex) {

  operationStatus.current.setPrecisionStatus(precisionIndex);

}

#endif // OPERATIONS_H
