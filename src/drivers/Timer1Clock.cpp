#include "Timer1Clock.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <TimingCore.h>

// Relè su D47 = PORTL bit PL2. PORTL è fuori dallo spazio SBI/CBI e tone() scrive
// PL0 dalla sua ISR: dal main il relè si scrive solo in ATOMIC_BLOCK.
#define RELAY_ON() (PORTL |= _BV(PL2))
#define RELAY_OFF() (PORTL &= ~_BV(PL2))

static TimingCore core;

// TimingCore::tick() è inline: niente chiamate né divisioni nella ISR.
ISR(TIMER1_COMPA_vect) {
  if (core.tick()) RELAY_OFF();
}

void Timer1Clock::begin() {
  RELAY_OFF();
  DDRL |= _BV(PL2);
  // 16 MHz / 64 = 4 us per conteggio, OCR1A = 249 -> 1 ms
  TCCR1A = 0;
  TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
  TCNT1 = 0;
  OCR1A = 249;
  TIMSK1 |= _BV(OCIE1A);
}

void Timer1Clock::startCountdown(uint32_t durationMs) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    core.startCountdown(durationMs);
    RELAY_ON();
  }
}

void Timer1Clock::cancel() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    core.cancel();
    RELAY_OFF();
  }
}

bool Timer1Clock::running() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return core.run() != Run::Idle; }
  return false;  // irraggiungibile, evita il warning
}

uint32_t Timer1Clock::remainingMs() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return core.remainingMs(); }
  return 0;  // irraggiungibile, evita il warning
}
