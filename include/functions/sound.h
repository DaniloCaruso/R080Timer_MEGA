// SOUND_H

#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include <ArduinoSTL.h>
#include "variables.h"

// PROTOTYPES
void emitSound(int toneType, int duration);

// IMPLEMENTATIONS
void emitSound(int toneType, int duration) {
  !mute ? tone(BUZZER_PIN, toneType, duration) : noop();
}

void emitStopSound() {
  emitSound(TONE_DOWN, 75);
  delay(150);
  emitSound(TONE_DOWN, 75);
  delay(150);
  emitSound(TONE_DOWN, 150);
}


void emitMetronome(int arTime) {
  if (isActionStarted && arTime%10 == 0) {
    emitSound(TONE_DOWN, 80);
  }
}
#endif // SOUND_H