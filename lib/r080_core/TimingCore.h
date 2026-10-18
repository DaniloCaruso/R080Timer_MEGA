// Logica temporale eseguita a ogni tick (1 ms) della ISR di Timer1.
// Tutto inline: la ISR non deve contenere chiamate né divisioni.
#pragma once

#include <stdint.h>
#include "ClockCorrection.h"

enum class Run : uint8_t { Idle, Focus, Countdown, StripContinuous, StripStepped };

// Eventi di un tick, a bit: la ISR li accumula, il loop li raccoglie.
enum : uint8_t { EV_END = 1, EV_STEP = 2, EV_METRONOME = 4, EV_PREWARN = 8, EV_FOCUS_WARN = 16, EV_FOCUS_OFF = 32 };
struct TickOut {
  uint8_t events;
  bool relayOff;
};

constexpr uint32_t STRIP_LIMIT_MS = 999900;  // REQ-3.14
constexpr uint32_t NO_STEP = 0xFFFFFFFF;     // prossimo Step non ancora calcolato dal loop
constexpr uint32_t FOCUS_WARN_MS = 10000;    // REQ-3.4: avviso 10 s prima dello Spegnimento automatico

class TimingCore {
 public:
  void startCountdown(uint32_t durationMs) {
    elapsed_ = 0;
    msInSecond_ = 0;
    target_ = durationMs;
    run_ = Run::Countdown;
  }
  // limitMs > FOCUS_WARN_MS: i limiti del Focus partono da 3 minuti.
  void startFocus(uint32_t limitMs) {
    elapsed_ = 0;
    target_ = limitMs;
    warn_ = limitMs - FOCUS_WARN_MS;  // calcolato qui: nella ISR resta un confronto
    run_ = Run::Focus;
  }
  // Test strip: il loop calcola ogni Step (float, C-9) e lo passa con setNextStep().
  // Passo-passo: allo Step il core si ferma; la ripresa riparte da elapsedMs.
  void startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs) {
    elapsed_ = elapsedMs;
    next_ = nextStepMs;
    run_ = stepped ? Run::StripStepped : Run::StripContinuous;
  }
  void setNextStep(uint32_t nextStepMs) { next_ = nextStepMs; }
  void cancel() { run_ = Run::Idle; }
  void setCorrection(int16_t ppm) { corr_.set(ppm); }

  // REQ-1.7: la Correzione del clock fa contare a questo tick 0, 1 o 2 ms, e ogni ms passa
  // dalle stesse regole: nessun bordo saltato o ripetuto.
  TickOut tick() {
    TickOut out = {0, false};
    for (uint8_t n = corr_.step(); n; n--) {
      TickOut ms = tickMs();
      out.events |= ms.events;
      out.relayOff |= ms.relayOff;
    }
    return out;
  }

  uint32_t remainingMs() const { return run_ == Run::Countdown ? target_ - elapsed_ : 0; }
  uint32_t elapsedMs() const { return elapsed_; }
  Run run() const { return run_; }

 private:
  // Un ms di tempo contato.
  TickOut tickMs() {
    if (run_ == Run::StripContinuous || run_ == Run::StripStepped) return stripTick();
    if (run_ == Run::Focus) return focusTick();
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

  TickOut stripTick() {
    if (++elapsed_ >= STRIP_LIMIT_MS) {
      run_ = Run::Idle;
      return {EV_END, true};
    }
    if (elapsed_ >= next_) {
      next_ = NO_STEP;
      bool pause = run_ == Run::StripStepped;  // REQ-1.3: relè spento al tick dello Step
      if (pause) run_ = Run::Idle;
      return {EV_STEP, pause};
    }
    uint32_t left = next_ - elapsed_;
    return {static_cast<uint8_t>(left == 300 || left == 200 || left == 100 ? EV_PREWARN : 0), false};  // REQ-8.3
  }

  // REQ-3.3, REQ-3.4: nessun Metronomo in Focus (REQ-8.2).
  TickOut focusTick() {
    if (++elapsed_ >= target_) {
      run_ = Run::Idle;
      return {EV_FOCUS_OFF, true};
    }
    return {elapsed_ == warn_ ? uint8_t(EV_FOCUS_WARN) : uint8_t(0), false};
  }

  ClockCorrection corr_;
  Run run_ = Run::Idle;
  uint32_t elapsed_ = 0, target_ = 0, next_ = NO_STEP, warn_ = 0;
  uint16_t msInSecond_ = 0;
};
