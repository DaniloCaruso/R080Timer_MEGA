#pragma once
#include <Hal.h>

// Buzzer finto: registra ogni nota con l'istante in cui è partita.
struct FakeBuzzer : Buzzer {
  struct Note { uint32_t atMs; uint16_t hz, ms; };
  const uint32_t& nowMs;
  Note notes[64];
  uint8_t count = 0;

  explicit FakeBuzzer(const uint32_t& now) : nowMs(now) {}
  void play(uint16_t hz, uint16_t ms) override {
    if (count < 64) notes[count++] = {nowMs, hz, ms};
  }
};
