#include <Arduino.h>
#include "variables.h"
#include "functions/base.h"
#include "functions/sound.h"
#include "functions/presentation.h"
#include "functions/operations.h"
#include "types.h"

// ACTIONS_H
#ifndef ACTIONS_H
#define ACTIONS_H

void stopCountDown() {
  relayStart(false);
  isActionStarted = false;
  isMainBtnPressed = false;
  timeSet = appoTimeSet;
  emitStopSound();
  showTime(timeSet);
}

void initTestStrip() {
    testStripTime[channel] = appoTimeSet;
    relayStart(true);
    isActionStarted = true;
    nextTimeSet = timeSet < 10 ? 10 : timeSet;
    timeSet = 0;
}

void initCountdown() {
    countdownTime[channel] = appoTimeSet;
    relayStart(true); 
    isActionStarted = true;
    lastTimeSet = millis();
}

void initFocus() {
  relayStart(true);
  isActionStarted = true;
  lastTimeSet = millis();
}

void focus() {
  int currentTimeSet = millis();
  int timePassed = (currentTimeSet - lastTimeSet);
  if (!isActionStarted) { initFocus(); }
  if (timePassed >= 100) {
    lastTimeSet = millis();
    timeSet++;
    showTime(timeSet);
  }
}

void testStrip() {
  if (!isActionStarted) { 
    initTestStrip();
  }

  int currentTimeSet = millis();
  int timePassed = (currentTimeSet - lastTimeSet);
  int precisionSet = operationStatus.current.precisionStatus.current.value;


  if (timePassed >= 100) {
    lastTimeSet = millis();
    timeSet++;
    showTime(timeSet);
    if (timeSet == nextTimeSet) {
      emitSound(TONE_UP, 150); 

      switch(operationStatus.current.precisionTypeStatus.current.id) {
        case PrecisionType::FSTOP:
          multSet = pow(2.0,(1.0/operationStatus.current.precisionStatus.current.value));  
          nextTimeSet = int(((float(timeSet) * multSet) * 10 + 5)/10);
          break;
        default: 
          nextTimeSet = timeSet + precisionSet * 10;
          break;
      }
     
      if (operationStatus.current.modalityTypeStatus.current.id == ModalityType::STEP_BY_STEP) {
        relayStart(false);
        isActionStarted = false;
        delay(500);
        timeSet = nextTimeSet;
        showTime(timeSet);
      }
    }
    
    if (nextTimeSet - timeSet <= 3) {
      emitSound(TONE_UP, 50);
    }
  }
}

void countdown() {

  if (!isActionStarted) { initCountdown(); }

  int currentTimeSet = millis();
  int timePassed = (currentTimeSet - lastTimeSet);

  if (timeSet <= 0) { stopCountDown(); }

  if (timePassed >= 100) {
    lastTimeSet = millis();
    timeSet--;
    emitMetronome(timeSet);
    showTime(timeSet);
  }
}

#endif //ACTIONS_H