// Logica temporale eseguita a ogni tick (1 ms) della ISR di Timer1.
// Tutto inline: la ISR non deve contenere chiamate né divisioni.
#pragma once

#include <stdint.h>

enum class Run : uint8_t { Idle, Countdown };

class TimingCore {
 public:
  void startCountdown(uint32_t durationMs) { elapsed_ = 0; target_ = durationMs; run_ = Run::Countdown; }
  void cancel() { run_ = Run::Idle; }

  // Ritorna true nel tick in cui il relè va spento.
  bool tick() {
    if (run_ != Run::Countdown) return false;
    if (++elapsed_ < target_) return false;
    run_ = Run::Idle;
    return true;
  }

  uint32_t remainingMs() const { return run_ == Run::Countdown ? target_ - elapsed_ : 0; }
  Run run() const { return run_; }

 private:
  Run run_ = Run::Idle;
  uint32_t elapsed_ = 0, target_ = 0;
};
