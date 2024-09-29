#ifndef TIMER_MANAGER_H
#define TIMER_MANAGER_H

#include <Arduino.h>
#include <util/atomic.h>
#include "variables.h"
#include "functions/base.h"
#include "functions/sound.h"
#include "functions/presentation.h"

// Stato del timer main-side (file-scope).
// timerCurrentValue e nextStepTarget restano in decisecondi come in precedenza
// per preservare i contratti con evaluateKey/operations.
static int timerCurrentValue = 0;
static int nextStepTarget = 0;

// === Start / Stop ===

inline void startFocusTimer() {
  timerCurrentValue = 0;
  relayStart(true);
  isActionStarted = true;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_elapsedMs = 0;
    g_remainingMs = 0;
    g_relayGate = channelsRelayActivated[channel - 1];
    g_modeActive = 1;
  }
}

inline void startCountdownTimer() {
  // appoTimeSet<=0: ISR.case2 non decrementerebbe mai (guardia g_remainingMs>0)
  // e relay resterebbe ON fino a stop manuale. Aborto silente.
  if (appoTimeSet <= 0) return;
  timerCurrentValue = appoTimeSet;
  relayStart(true);
  isActionStarted = true;
  showTime(timerCurrentValue);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_remainingMs = (uint32_t)appoTimeSet * 100UL;
    g_elapsedMs = 0;
    g_relayGate = channelsRelayActivated[channel - 1];
    g_modeActive = 2;
  }
}

inline void startTestStripTimer() {
  // Fresh-start se g_elapsedMs==0 (dopo resetTimerState o primo avvio);
  // resume STEP_BY_STEP preserva g_elapsedMs e g_nextStepTargetMs intatti.
  uint32_t elapsedSnap;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { elapsedSnap = g_elapsedMs; }
  if (elapsedSnap == 0) {
    nextStepTarget = appoTimeSet > 0 ? appoTimeSet : 10;
    timerCurrentValue = 0;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      g_elapsedMs = 0;
      g_nextStepTargetMs = (uint32_t)nextStepTarget * 100UL;
    }
  }
  relayStart(true);
  isActionStarted = true;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_relayGate = channelsRelayActivated[channel - 1];
    g_modeActive = 3;
  }
}

inline void stopAllTimers() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { g_modeActive = 0; }
  relayStart(false);
  isActionStarted = false;
  emitStopSound();
  showTime(timerCurrentValue);
}

inline void resetTimerState() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_modeActive = 0;
    g_elapsedMs = 0;
    g_remainingMs = 0;
    g_nextStepTargetMs = 0;
  }
  timerCurrentValue = 0;
  nextStepTarget = 0;
  isActionStarted = false;
}

// === Handler main-side ===
//
// Il loop principale legge in modo atomico i flag alzati dalla ISR e invoca
// l'handler corrispondente. Dopo Fase D, metronomo e step beep sono
// dispatched direttamente dalla ISR: onTick100ms si limita a rendering,
// sync stato legacy e warning beep pre-step (che resta polled perche'
// basato sulla distanza in decis, non sul tick al ms).

inline void onTick100ms() {
  uint8_t mode;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { mode = g_modeActive; }

  if (mode == 1) { // FOCUS
    timerCurrentValue++;
    showTime(timerCurrentValue);
    timeSet = timerCurrentValue;
    return;
  }

  if (mode == 2) { // COUNTDOWN — progresso e fine sono guidati dall'ISR
    uint32_t remMs;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { remMs = g_remainingMs; }
    int decis = (int)(remMs / 100UL);
    timerCurrentValue = decis;       // sync stato legacy per stopAllTimers
    showTime(decis);
    return;
  }

  if (mode == 3) { // TEST_STRIP — g_elapsedMs authoritative da ISR
    uint32_t elapsedMs;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { elapsedMs = g_elapsedMs; }
    timerCurrentValue = (int)(elapsedMs / 100UL);
    showTime(timerCurrentValue);
    timeSet = timerCurrentValue;

    // Warning beep 3 decis prima dello step (resta polled, si basa su
    // distanza in decis e sfrutta la cadenza di rendering a 100 ms).
    int distToStep = nextStepTarget - timerCurrentValue;
    if (distToStep > 0 && distToStep <= 3) {
      emitSound(TONE_UP, 50);
    }
  }
}

// Fase B: l'ISR esegue il flip PORT del rele' esattamente al ms di fine
// countdown. Qui solo cleanup non critico (stato, stop sound, repeat last
// exposure, render finale). Il rele' e' gia' OFF quando entriamo qui.
inline void onCountdownEnd() {
  isActionStarted = false;
  isMainBtnPressed = false;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_modeActive = 0;
    g_remainingMs = 0;
  }
  emitStopSound();
  timeSet = appoTimeSet;              // repeat-last-exposure
  timerCurrentValue = 0;              // sync stato legacy
  showTime(0);
}

// Fase D: la ISR alza g_nextStepReached quando g_elapsedMs>=g_nextStepTargetMs.
// Qui emettiamo lo step beep (TONE_UP 150ms), ricalcoliamo il prossimo target
// in base a precisione (FSTOP/SECONDS) e modalita' (CONTINOUS/STEP_BY_STEP),
// aggiorniamo atomicamente g_nextStepTargetMs per l'ISR e, in STEP_BY_STEP,
// spegniamo rele' e azzeriamo g_modeActive per fermare il conteggio fino al
// prossimo click. Guard iniziale: se la flag e' spuria (elapsed < target per
// aggiornamento target avvenuto tra clear-flag e processing), return silente.
inline void onTestStripStep() {
  uint32_t elapsedMs, targetMs;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    elapsedMs = g_elapsedMs;
    targetMs  = g_nextStepTargetMs;
  }
  if (elapsedMs < targetMs) return;

  int timerCurrentDecis = (int)(elapsedMs / 100UL);
  emitSound(TONE_UP, 150);

  int precisionValue = operationStatus.current.precisionStatus.current.value;
  int newTargetDecis;

  if (operationStatus.current.precisionTypeStatus.current.id == FSTOP) {
    float multiplier = pow(2.0, 1.0 / (float)precisionValue);
    newTargetDecis = (int)((float)timerCurrentDecis * multiplier + 0.5f);
    if (newTargetDecis <= timerCurrentDecis) newTargetDecis = timerCurrentDecis + 1;
  } else {
    newTargetDecis = timerCurrentDecis + precisionValue * 10;
  }
  if (newTargetDecis > 9999) newTargetDecis = 9999;
  if (newTargetDecis < 1)    newTargetDecis = 1;

  nextStepTarget = newTargetDecis;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_nextStepTargetMs = (uint32_t)newTargetDecis * 100UL;
  }

  if (operationStatus.current.modalityTypeStatus.current.id == STEP_BY_STEP) {
    RELAY_OFF();
    isActionStarted = false;
    isMainBtnPressed = false;
    showTime(newTargetDecis);
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { g_modeActive = 0; }
  }
}

// Fase D: metronomo ISR-driven. La ISR alza g_metronomeFlag ogni 1000 ms
// esatti durante il COUNTDOWN (jitter < 1 ms). Qui emettiamo il tick.
// emitSound e' gia' no-op in mute; il controllo esplicito !mute e' ridondante
// ma documenta l'intent ed e' esplicitato dall'acceptance criteria.
inline void onMetronomeBeat() {
  if (!mute) emitSound(TONE_DOWN, 80);
}

#endif // TIMER_MANAGER_H
