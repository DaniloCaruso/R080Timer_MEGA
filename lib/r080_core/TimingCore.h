// Logica temporale eseguita a ogni tick (1 ms) della ISR di Timer1.
// Tutto inline: la ISR non deve contenere chiamate né divisioni.
#pragma once

#include <stdint.h>

enum class Run : uint8_t { Idle, Countdown, StripContinuous };

// Eventi di un tick, a bit: la ISR li accumula, il loop li raccoglie.
enum : uint8_t { EV_END = 1, EV_STEP = 2, EV_METRONOME = 4, EV_PREWARN = 8 };
struct TickOut {
  uint8_t events;
  bool relayOff;
};

constexpr uint32_t STRIP_LIMIT_MS = 999900;  // REQ-3.14
constexpr uint32_t NO_STEP = 0xFFFFFFFF;     // prossimo Step non ancora calcolato dal loop

class TimingCore {
 public:
  void startCountdown(uint32_t durationMs) {
    elapsed_ = 0;
    msInSecond_ = 0;
    target_ = durationMs;
    run_ = Run::Countdown;
  }
  // Test strip: il loop calcola ogni Step (float, C-9) e lo passa con setNextStep().
  void startStrip(uint32_t firstStepMs) {
    elapsed_ = 0;
    next_ = firstStepMs;
    run_ = Run::StripContinuous;
  }
  void setNextStep(uint32_t nextStepMs) { next_ = nextStepMs; }
  void cancel() { run_ = Run::Idle; }

  TickOut tick() {
    if (run_ == Run::StripContinuous) return stripTick();
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
  uint32_t elapsedMs() const { return elapsed_; }
  Run run() const { return run_; }

 private:
  TickOut stripTick() {
    if (++elapsed_ >= STRIP_LIMIT_MS) {
      run_ = Run::Idle;
      return {EV_END, true};
    }
    if (elapsed_ >= next_) {
      next_ = NO_STEP;
      return {EV_STEP, false};
    }
    uint32_t left = next_ - elapsed_;
    return {static_cast<uint8_t>(left == 300 || left == 200 || left == 100 ? EV_PREWARN : 0), false};  // REQ-8.3
  }

  Run run_ = Run::Idle;
  uint32_t elapsed_ = 0, target_ = 0, next_ = NO_STEP;
  uint16_t msInSecond_ = 0;
};
