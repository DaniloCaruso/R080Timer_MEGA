// SOUND_H

#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include "variables.h"
#include "functions/base.h"

// PROTOTYPES
void emitSound(int toneType, int duration);

// IMPLEMENTATIONS
void emitSound(int toneType, int duration) {
  !mute ? tone(BUZZER_PIN, toneType, duration) : noop();
}

#endif // SOUND_H