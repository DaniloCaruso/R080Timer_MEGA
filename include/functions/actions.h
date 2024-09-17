// ACTIONS_H
#ifndef ACTIONS_H
#define ACTIONS_H

#include <Arduino.h>
#include "types.h"
#include "variables.h"
#include "functions/base.h"
#include "functions/sound.h"
#include "functions/presentation.h"
#include "functions/running_mgmt.h"

void stopAction() {
    releStartStop(false);
    setActionStarted(false);
    timeSet = appoTimeSet;
    emitStopSound();
    showTime(timeSet);
}

void countdown() {
  if (timeSet < 0) { stopAction(); }
  if (timeSet > 0 && (millis() - lastTimeSet >= 100)) {
    lastTimeSet = millis();
    timeSet--;
    emitMetronome(timeSet);
    showTime(timeSet);
    if (timeSet == 0) { stopAction(); }
  }
}

void strip() {
  if (millis() - lastTimeSet >= 100) {
    lastTimeSet = millis();
    timeSet++;
    showTime(timeSet);
    if (timeSet == nextTimeSet) {
      emitSound(TONE_UP, 150); 

      if (operationStatus.current.precisionType.id == Precision::FSTOP) {   
        multSet = pow(2.0,(1.0/precisionSet));  
        //time_succ = int(float(time) * mult);
        nextTimeSet = int(((float(timeSet) * multSet) * 10 + 5)/10);
      } else {
        nextTimeSet = timeSet + precisionSet * 10;
      }

      if (operationStatus.current.modalityType.id == Modality::STEP_BY_STEP) {
        releStartStop(false);
        setActionStarted(false);
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

OperationActions operationActions[3] = {
    OperationActions(
      []() -> void { 
          releStartStop(true);
          setActionStarted(true);
      },
      []() -> void { }
    ), 
  // 0 => FOCUS, 1 => COUNTDOWN, 2 => TEST_STRIP
    OperationActions(
        []() -> void { 
            countdownTime[channel] = appoTimeSet;
            if (timeSet > 0) { releStartStop(true); 
                setActionStarted(true);
                lastTimeSet = millis();
                countdown();
            }
        },
        []() -> void { countdown(); }
    ),
    OperationActions(
        []() -> void {
            testStripTime[channel] = appoTimeSet;
            releStartStop(true);
            setActionStarted(true);
            nextTimeSet = timeSet < 10 ? 10 : timeSet;
            timeSet = 0;
            strip();
        },
        []() -> void { strip(); }
    )
 };

#endif //ACTIONS_H