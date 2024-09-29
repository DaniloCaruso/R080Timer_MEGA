#ifndef ACTIONS_H
#define ACTIONS_H

#include <Arduino.h>
#include "variables.h"
#include "functions/timer_manager.h"

void focus() {
  if (!isActionStarted) {
    startFocusTimer();
  }
}

void countdown() {
  if (!isActionStarted) {
    startCountdownTimer();
  }
}

void testStrip() {
  if (!isActionStarted) {
    startTestStripTimer();
  }
}

#endif // ACTIONS_H
