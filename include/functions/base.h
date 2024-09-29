// BASE_H
#ifndef BASE_H
#define BASE_H

#include <Arduino.h>
#include "variables.h"

// PROTOTYPES
void noop();
void relayStart(boolean start);

// IMPLEMENTATIONS
void noop() { }

void relayStart(boolean start) {
    if (channelsRelayActivated[channel - 1]) {
      if (start) { RELAY_ON(); } else { RELAY_OFF(); }
    }
}

#endif // BASE_H
