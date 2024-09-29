
// BASE_H
#ifndef BASE_H
#define BASE_H

#include <Arduino.h>
#include <ArduinoSTL.h>
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

void relayStart(boolean start) {

    if (channelsRelayActivated[channel]) {
      digitalWrite(ENLARGER_RELAY_PIN, start ? HIGH : LOW);
    }
}





#endif // BASE_H