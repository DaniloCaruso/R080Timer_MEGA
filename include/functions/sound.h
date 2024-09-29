// SOUND_H

#ifndef SOUND_H
#define SOUND_H

#include <Arduino.h>
#include "variables.h"

// Struttura per eventi sonori non-bloccanti
struct SoundEvent {
  int frequency;       // Hz, 0 = pausa silenziosa
  unsigned int duration; // ms
};

struct SoundPlayer {
  bool active;
  const SoundEvent* sequence;
  byte length;
  byte index;
  unsigned long startedAt;
};

// Sequenze predefinite
const SoundEvent STOP_SEQUENCE[] = {
  {TONE_DOWN, 75}, {0, 150}, {TONE_DOWN, 75}, {0, 150}, {TONE_DOWN, 150}
};
const byte STOP_SEQUENCE_LEN = 5;

// Player globale
SoundPlayer soundPlayer = { false, nullptr, 0, 0, 0 };

// PROTOTYPES
void emitSound(int toneType, int duration);
void emitStopSound();
void tickSound();

// IMPLEMENTATIONS

// Suono singolo (tone() e' gia' non-bloccante)
void emitSound(int toneType, int duration) {
  if (!mute) {
    tone(BUZZER_PIN, toneType, duration);
  }
}

// Avvia sequenza di stop (non-bloccante)
void emitStopSound() {
  if (soundPlayer.active) return; // non interrompere sequenza in corso
  soundPlayer.sequence = STOP_SEQUENCE;
  soundPlayer.length = STOP_SEQUENCE_LEN;
  soundPlayer.index = 0;
  soundPlayer.startedAt = readTickMs();
  soundPlayer.active = true;
  // Avvia primo evento
  if (STOP_SEQUENCE[0].frequency > 0 && !mute) {
    tone(BUZZER_PIN, STOP_SEQUENCE[0].frequency, STOP_SEQUENCE[0].duration);
  }
}

// Tick del sound player — chiamare in loop()
void tickSound() {
  if (!soundPlayer.active) return;
  if (readTickMs() - soundPlayer.startedAt < soundPlayer.sequence[soundPlayer.index].duration) return;

  // Evento corrente terminato, avanza
  soundPlayer.index++;
  if (soundPlayer.index >= soundPlayer.length) {
    soundPlayer.active = false;
    noTone(BUZZER_PIN);
    return;
  }

  // Avvia prossimo evento
  soundPlayer.startedAt = readTickMs();
  const SoundEvent& evt = soundPlayer.sequence[soundPlayer.index];
  if (evt.frequency > 0 && !mute) {
    tone(BUZZER_PIN, evt.frequency, evt.duration);
  } else {
    noTone(BUZZER_PIN);
  }
}

#endif // SOUND_H
