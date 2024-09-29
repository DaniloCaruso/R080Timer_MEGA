// TIMER_ISR_H
// Motore temporale hardware: Timer1 in modalita CTC, ISR a 1 kHz.
// Unica sorgente temporale del firmware (sostituisce contrem/arduino-timer).
//
// F_CPU = 16 MHz, prescaler 64 -> 4 us per tick, OCR1A = 249 -> 1 ms/ISR.
//
// Fase B: aggiunte macro PORT-level per il rele' (flip atomico SBI/CBI) e
// logica COUNTDOWN direttamente in ISR. Il rele' viene aperto al bordo
// preciso del ms quando g_remainingMs raggiunge 0, indipendentemente dal
// carico del main loop. Handler main-side onCountdownEnd esegue solo
// cleanup non critico (stato, suono, display).
//
// Fase D: trigger ISR-driven per metronomo e step beep test-strip. In
// COUNTDOWN viene alzato g_metronomeFlag ogni 1000 ms esatti (tick al
// secondo con jitter < 1 ms), e in TEST_STRIP g_elapsedMs viene
// incrementato in ISR; g_nextStepReached scatta quando supera il target,
// per beep di step sincronizzati al ms.

#ifndef TIMER_ISR_H
#define TIMER_ISR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include "variables.h"

// Pin rele' ingranditore: Arduino D47 = PORTL bit 2 (PL2) sull'ATmega2560.
// Verifica: ATmega2560 datasheet sezione "I/O Ports - Port L" (PL0..PL7
// mappati a D49..D42) e tabella pinout Arduino Mega 2560 (digital pin 47
// -> port L bit 2). PORTL e' memory-mapped all'indirizzo 0x10B (extended
// I/O, fuori dal range 0x00..0x1F): GCC NON puo' emettere SBI/CBI ma una
// sequenza lds/andi (o ori) /sts non atomica. Sicuro dentro l'ISR perche'
// SREG.I=0 disabilita le altre interrupt; dal main e' sicuro per PL2
// perche' nessun'altra ISR scrive PL2 e gli altri bit di PORTL toccati
// sono PL0 (buzzer via Timer2 tone()) — race solo audio, irrilevante.
#define RELAY_PORT  PORTL
#define RELAY_BIT   PL2
#define RELAY_ON()  (RELAY_PORT |=  (1 << RELAY_BIT))
#define RELAY_OFF() (RELAY_PORT &= ~(1 << RELAY_BIT))

static volatile uint8_t g_100Counter = 0;

// Lettura atomica del clock Timer1 ISR in ms. Sorgente temporale unificata
// per tutto il firmware (sostituisce millis()/Timer0 per sottosistemi come
// SoundPlayer), eliminando drift tra Timer0 e Timer1.
inline uint32_t readTickMs() {
  uint32_t snapshot;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    snapshot = g_tickMs;
  }
  return snapshot;
}

inline void setupTimer1() {
  cli();
  TCCR1A = 0;                           // modalita CTC, no output compare pins
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 249;                         // 250 tick * 4 us = 1 ms
  TCCR1B |= (1 << WGM12);               // CTC (TOP = OCR1A)
  TCCR1B |= (1 << CS11) | (1 << CS10);  // prescaler 64
  TIMSK1 |= (1 << OCIE1A);              // abilita interrupt Compare Match A
  sei();
}

ISR(TIMER1_COMPA_vect) {
  g_tickMs++;

  switch (g_modeActive) {
    case 2: // COUNTDOWN
      if (g_remainingMs > 0) {
        g_remainingMs--;
        if (g_remainingMs == 0) {
          if (g_relayGate) RELAY_OFF();
          g_endOfCountdown = 1;
        } else if ((g_remainingMs % 1000) == 0) {
          g_metronomeFlag = 1;
        }
      }
      break;
    case 3: // TEST_STRIP
      g_elapsedMs++;
      if (g_elapsedMs >= g_nextStepTargetMs) {
        g_nextStepReached = 1;
      }
      break;
    // case 1 (FOCUS) continua ad usare solo il tick 100 ms main-side.
  }

  if (++g_100Counter >= 100) {
    g_100Counter = 0;
    g_tickFlag100ms = 1;
  }
}

#endif // TIMER_ISR_H
