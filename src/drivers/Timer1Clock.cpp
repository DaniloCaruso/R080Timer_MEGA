#include "Timer1Clock.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <TimingCore.h>
#include "Pins.h"

static TimingCore core;
static volatile uint8_t events;  // EV_* accumulati fino a takeEvents()

// TimingCore::tick() è inline: niente chiamate né divisioni nella ISR.
ISR(TIMER1_COMPA_vect) {
  TickOut out = core.tick();
  events |= out.events;
  if (out.relayOff) RELAY_OFF();
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

void Timer1Clock::startCountdown(uint32_t durationMs, bool lamp) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    core.startCountdown(durationMs);
    if (lamp) RELAY_ON();
  }
}

void Timer1Clock::startFocus(uint32_t limitMs, bool lamp) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    core.startFocus(limitMs);
    if (lamp) RELAY_ON();
  }
}

void Timer1Clock::startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs, bool lamp) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    core.startStrip(stepped, elapsedMs, nextStepMs);
    if (lamp) RELAY_ON();
  }
}

void Timer1Clock::setNextStep(uint32_t nextStepMs) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { core.setNextStep(nextStepMs); }
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

uint32_t Timer1Clock::elapsedMs() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { return core.elapsedMs(); }
  return 0;  // irraggiungibile, evita il warning
}

uint8_t Timer1Clock::takeEvents() {
  uint8_t e = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {  // leggere e azzerare senza perdere un evento della ISR
    e = events;
    events = 0;
  }
  return e;
}
