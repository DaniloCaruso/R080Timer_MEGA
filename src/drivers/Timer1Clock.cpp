#include "Timer1Clock.h"

#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/atomic.h>
#include <TimingCore.h>
#include "Pins.h"

static TimingCore core;
static volatile uint8_t events;  // EV_* accumulati fino a takeEvents()
static volatile uint32_t ticks;  // ISR eseguite dall'avvio
static volatile uint8_t isrMax;  // massimo di TCNT1 all'uscita della ISR, in conteggi da 4 us

// TimingCore::tick() è inline: niente chiamate né divisioni nella ISR.
ISR(TIMER1_COMPA_vect) {
  TickOut out = core.tick();
  events |= out.events;
  if (out.relayOff) RELAY_OFF();
  ticks++;
  // REQ-9.4: Timer1 riparte da 0 al compare match, quindi TCNT1 qui è la durata della ISR.
  uint8_t t = TCNT1L;  // sotto 250: basta il byte basso
  if (t > isrMax) isrMax = t;
}

void Timer1Clock::begin() {
  RELAY_OFF();
  DDRL |= _BV(PL2);
  // 16 MHz / 64 = 4 us per conteggio, OCR1A = 249 -> 1 ms
  TCCR1A = 0;
  TCCR4A = 0;  // init() di Arduino lo lascia in PWM: qui conteggio libero (REQ-9.4)
  TCCR4B = 0;
  TCNT1 = 0;
  TCNT4 = 0;
  OCR1A = 249;
  TIMSK1 |= _BV(OCIE1A);
  // Stesso prescaler 64, condiviso fra i timer: i due conteggi partono insieme e restano in passo.
  TCCR1B = _BV(WGM12) | _BV(CS11) | _BV(CS10);
  TCCR4B = _BV(CS41) | _BV(CS40);
}

// Timer4 va a capo ogni 262 ms: il loop lo legge prima (il watchdog lo ferma a 250 ms).
void Timer1Clock::measure(Health& h) {
  uint16_t t4;
  uint32_t n;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    t4 = TCNT4;
    n = ticks;
  }
  refCounts_ += uint16_t(t4 - lastT4_);
  lastT4_ = t4;
  h.isrMaxUs = isrMax * 4UL;
  if (refCounts_ < 250000UL) return;  // il confronto ogni secondo di riferimento
  expected_ += refCounts_ / 250;
  refCounts_ %= 250;
  // Una ISR in attesa o mezzo conteggio di sfasamento valgono un tick: oltre, sono tick persi.
  int32_t lag = int32_t(expected_ - n);
  if (lag > 1) h.lostTicks = lag - 1;
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

void Timer1Clock::setRelay(bool on) {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {  // PORTL: vedi Pins.h
    if (on) RELAY_ON();
    else RELAY_OFF();
  }
}

uint8_t Timer1Clock::takeEvents() {
  uint8_t e = 0;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {  // leggere e azzerare senza perdere un evento della ISR
    e = events;
    events = 0;
  }
  return e;
}
