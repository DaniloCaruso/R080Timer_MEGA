// Suoni del timer: sequenze di note dalla tabella kSounds (SoundScheduler.cpp),
// suonate senza attese dal loop. Una sequenza ne interrompe una di priorità
// minore o uguale.
#pragma once

#include <stdint.h>
#include "Hal.h"

// In ordine di priorità, dalla più alta.
enum class Sound : uint8_t { End, Error, Start, Metronome, Confirm };

class SoundScheduler {
 public:
  explicit SoundScheduler(Buzzer& buzzer) : buzzer_(buzzer) {}
  void play(Sound s);
  void tick(uint32_t nowMs);  // a ogni giro di loop

 private:
  static constexpr uint8_t IDLE = 0xFF;
  Buzzer& buzzer_;
  Sound cur_ = Sound::Start;
  uint8_t step_ = IDLE;  // prossima nota della sequenza in corso
  bool startNow_ = false;
  uint32_t nextMs_ = 0;
};
