// Logica temporale eseguita a ogni tick (1 ms) della ISR di Timer1.
// Tutto inline: la ISR non deve contenere chiamate né divisioni.
#pragma once

#include <stdint.h>

enum class Run : uint8_t { Idle, Countdown };

// Eventi di un tick, a bit: la ISR li accumula, il loop li raccoglie.
enum : uint8_t { EV_END = 1, EV_METRONOME = 4 };
struct TickOut {
  uint8_t events;
  bool relayOff;
};

class TimingCore {
 public:
  void startCountdown(uint32_t durationMs) {
    elapsed_ = 0;
    msInSecond_ = 0;
    target_ = durationMs;
    run_ = Run::Countdown;
  }
  void cancel() { run_ = Run::Idle; }

  TickOut tick() {
    if (run_ != Run::Countdown) return {0, false};
    if (++elapsed_ >= target_) {
      run_ = Run::Idle;
      return {EV_END, true};
    }
    // REQ-1.6: contatore a 16 bit dall'inizio dell'Esposizione, niente modulo sul residuo.
    if (++msInSecond_ < 1000) return {0, false};
    msInSecond_ = 0;
    return {EV_METRONOME, false};
  }

  uint32_t remainingMs() const { return run_ == Run::Countdown ? target_ - elapsed_ : 0; }
  Run run() const { return run_; }

 private:
  Run run_ = Run::Idle;
  uint32_t elapsed_ = 0, target_ = 0;
  uint16_t msInSecond_ = 0;
};
