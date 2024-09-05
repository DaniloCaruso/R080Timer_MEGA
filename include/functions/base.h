
// BASE_H
#ifndef BASE_H
#define BASE_H

#include <Arduino.h>
#include "variables.h"

// PROTOTYPES
void noop();
void noopArgs(...);
void setErrlet();
boolean isInError();

// IMPLEMENTATIONS
void noop() { }

void noopArgs(...) { }

void setErrlet() {
  errlet = millis();
}

boolean isInError() {
  return (millis() - errlet) <= 20;
}

void updateTimerMode(int index) { }



#endif // BASE_H