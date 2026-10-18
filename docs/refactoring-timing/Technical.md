# Technical — Refactoring Timing R080 Timer MEGA

**Documento:** Design tecnico dettagliato
**Target hardware:** ATmega2560 @ 16 MHz
**Branch target:** `Performance_Rewrite` (da `Libraries_Refactoring`)
**Data:** 2026-04-18

---

## 1. Overview

Il motore temporale attuale (`contrem/arduino-timer`) è basato su polling: `appTimer.tick()` invocato a ogni giro di `loop()` legge `millis()` e invoca i callback scaduti. Il jitter effettivo è pari al worst-case di `loop()`.

Sostituzione: **Timer1 in modalità CTC (Clear Timer on Compare Match)** con ISR a 1 kHz. L'ISR è l'unica sorgente temporale del firmware; il main loop diventa event-driven su flag `volatile`.

### Clock budget (ATmega2560 @ 16 MHz)

- Periodo clock: 62.5 ns
- Periodo ISR: 1 ms = 16 000 cicli di clock
- Budget ISR target: < 20 µs = 320 cicli → 2% della CPU
- Margine per main loop: > 99% della CPU

### Uso dei Timer hardware (post-intervento)

| Timer | Uso | Note |
|---|---|---|
| Timer0 | `millis()`, `delay()`, `micros()` — Arduino core | Non toccato |
| Timer1 | Tick ISR 1 kHz (NUOVO) | Nostro |
| Timer2 | `tone()` / `noTone()` — Arduino core | Non toccato |
| Timer3 | Libero | Riserva |
| Timer4 | Libero | Riserva |
| Timer5 | Libero | Riserva |

---

## 2. Timer1 setup

### Calcolo OCR1A

Obiettivo: interrupt ogni 1 ms.

```
F_CPU = 16 000 000 Hz
Prescaler = 64
F_timer_tick = 16 MHz / 64 = 250 kHz
Periodo tick = 4 µs
Ticks per 1 ms = 250
OCR1A = 250 - 1 = 249
```

### Registri

```cpp
// include/functions/timer_isr.h  (NUOVO)
#ifndef TIMER_ISR_H
#define TIMER_ISR_H

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>

inline void setupTimer1() {
  cli();
  TCCR1A = 0;                          // modalità normale, no OC output
  TCCR1B = 0;
  TCNT1  = 0;
  OCR1A  = 249;                        // 250 tick * 4 µs = 1 ms
  TCCR1B |= (1 << WGM12);              // CTC mode (TOP=OCR1A)
  TCCR1B |= (1 << CS11) | (1 << CS10); // prescaler 64
  TIMSK1 |= (1 << OCIE1A);             // abilita interrupt su compare A
  sei();
}

#endif
```

### Integrazione

```cpp
// src/main.cpp
void setup() {
  Serial.begin(9600);
  digitalWrite(ENLARGER_RELAY_PIN, LOW);
  initializePinMode();
  configKeypad();
  configLCD();
  config7SegmentsDisplay();
  showWelcomeMessage();
  delay(3000);

  if (loadSettings()) {
    config7SegmentsDisplay();
  }

  reset7SegmentsDisplay();
  resetLCD();
  initializeOperations();
  registerClickHandler();
  configureMainButton();

  showSound();
  showChannel();
  showTime(timeSet);
  setErrlet();

  setupTimer1();                       // NUOVO — ultima istruzione prima del banner

  Serial.println(F("=== R080 TIMER PRONTO ==="));
}
```

---

## 3. Stato condiviso ISR ↔ main

Tutte le variabili condivise sono `volatile`. Le variabili di larghezza > 8 bit richiedono `ATOMIC_BLOCK` per letture dal main.

```cpp
// include/variables.h  (aggiunte)
volatile uint32_t g_tickMs          = 0;  // contatore monotonico
volatile uint32_t g_remainingMs     = 0;  // residuo countdown
volatile uint32_t g_elapsedMs       = 0;  // elapsed per focus/test-strip
volatile uint32_t g_nextStepTargetMs = 0; // target test-strip (ms assoluti)
volatile uint8_t  g_relayGate       = 0;  // snapshot canale attivo all'armo
volatile uint8_t  g_modeActive      = 0;  // 0=none, 1=focus, 2=countdown, 3=teststrip

volatile uint8_t  g_tickFlag100ms   = 0;  // set ogni 100 ISR
volatile uint8_t  g_endOfCountdown  = 0;  // set quando remaining→0
volatile uint8_t  g_nextStepReached = 0;  // set quando elapsed >= target step
volatile uint8_t  g_metronomeFlag   = 0;  // set ogni secondo in countdown

static volatile uint8_t g_100Counter = 0;  // privato ISR
```

**Nota:** `isActionStarted` esistente viene mantenuto (è `volatile boolean`) ma ora è alzato/abbassato in coordinamento con `g_modeActive`.

### Pin del relè — mappatura registro

Pin Arduino 47 sul Mega 2560 è **PL2** (PORT L bit 2).

```cpp
// Verifica da datasheet ATmega2560 § 13 Port L:
//   PL0 = D49, PL1 = D48, PL2 = D47, PL3 = D46, ...

#define RELAY_PORT   PORTL
#define RELAY_DDR    DDRL
#define RELAY_BIT    PL2

// Flip atomico (1 istruzione SBI/CBI):
#define RELAY_OFF()  (RELAY_PORT &= ~(1 << RELAY_BIT))
#define RELAY_ON()   (RELAY_PORT |=  (1 << RELAY_BIT))
```

---

## 4. L'ISR

```cpp
// include/functions/timer_isr.h  (continua)
ISR(TIMER1_COMPA_vect) {
  g_tickMs++;

  // Modo-specifico
  switch (g_modeActive) {

    case 2: // COUNTDOWN
      if (g_remainingMs > 0) {
        g_remainingMs--;

        // Metronomo: tick ogni secondo completo residuo
        if ((g_remainingMs % 1000) == 0) {
          g_metronomeFlag = 1;
        }

        // Fine esposizione: bordo preciso al µs
        if (g_remainingMs == 0) {
          if (g_relayGate) {
            RELAY_OFF();
          }
          g_endOfCountdown = 1;
        }
      }
      break;

    case 1: // FOCUS
      g_elapsedMs++;
      break;

    case 3: // TEST_STRIP
      g_elapsedMs++;
      if (g_elapsedMs >= g_nextStepTargetMs) {
        g_nextStepReached = 1;
      }
      break;
  }

  // Flag tick 100 ms per render e housekeeping
  if (++g_100Counter >= 100) {
    g_100Counter = 0;
    g_tickFlag100ms = 1;
  }
}
```

### Analisi performance ISR

Istruzioni per il worst-case path (COUNTDOWN attivo con metronomo e fine esposizione):

| Operazione | Cicli approssimativi |
|---|---|
| Prologo ISR (push registri) | ~30 |
| `g_tickMs++` (32-bit) | ~20 |
| `switch` | ~5 |
| `g_remainingMs--` (32-bit) | ~20 |
| Modulo + confronto metronomo | ~30 |
| Flip PORTL (SBI/CBI) | 2 |
| Flag assignment | ~4 |
| Increment `g_100Counter` + compare | ~8 |
| Epilogo ISR (pop + reti) | ~35 |
| **Totale worst case** | **~155 cicli = 9.7 µs** |

Budget di 20 µs rispettato con margine 2×. Compliant con REQ-2.1.

---

## 5. Main loop ristrutturato

```cpp
// src/main.cpp
void loop() {
  mainButton.tick();

  uint8_t flag100;
  uint8_t flagEnd;
  uint8_t flagStep;
  uint8_t flagMetro;

  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    flag100   = g_tickFlag100ms;  g_tickFlag100ms   = 0;
    flagEnd   = g_endOfCountdown; g_endOfCountdown  = 0;
    flagStep  = g_nextStepReached; g_nextStepReached = 0;
    flagMetro = g_metronomeFlag;  g_metronomeFlag   = 0;
  }

  if (flagEnd)   { onCountdownEnd(); }
  if (flagStep)  { onTestStripStep(); }
  if (flagMetro) { onMetronomeBeat(); }
  if (flag100)   { onTick100ms(); }

  tickSound();
  manageMainButtonAction();
}
```

### Ordine di processing dei flag

1. **`flagEnd`** prima di tutto: garantisce che la sequenza di stop suoni e il repeat-last-exposure avvengano prima di qualunque altro update (evita che il render 100ms mostri ancora il valore residuo "0").
2. **`flagStep`** prima di `flag100`: il beep di step deve precedere il render del nuovo target.
3. **`flagMetro`** prima di `flag100`: il beep del metronomo arriva prima dell'aggiornamento display, garantendo consistenza sonora.
4. **`flag100`** ultimo: update display non critici temporalmente.

### Handler handlers (snippet)

```cpp
// include/functions/timer_manager.h  (sostituisce vecchi callback arduino-timer)

inline void onCountdownEnd() {
  // Relè è già OFF (commutato in ISR). Qui solo cleanup non critico.
  isActionStarted = false;
  g_modeActive = 0;
  emitStopSound();
  timeSet = appoTimeSet;           // repeat-last-exposure
  showTime(0);                     // render 0 prima di tornare al reset
  showTime(timeSet);               // ripristina display
}

inline void onTestStripStep() {
  if (operationStatus.current.modality == STEP_BY_STEP) {
    if (g_relayGate) RELAY_OFF();
    isActionStarted = false;
    g_modeActive = 0;
  }
  emitSound(TONE_UP, 150);
  // calcola prossimo target (stessa logica attuale, ora in ms)
  uint32_t curDecis = g_elapsedMs / 100;
  uint32_t nextDecis = computeNextStripTarget(curDecis);
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_nextStepTargetMs = nextDecis * 100;
  }
  showTime(nextDecis);
}

inline void onMetronomeBeat() {
  if (!mute) emitSound(TONE_DOWN, 80);
}

inline void onTick100ms() {
  // Aggiornamento display per countdown/focus/test-strip
  uint32_t displayValue;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    if (g_modeActive == 2) displayValue = g_remainingMs / 100;
    else                   displayValue = g_elapsedMs / 100;
  }
  showTime((int)displayValue);
  // warning pre-step test-strip (3 decis pre-target)
  if (g_modeActive == 3) {
    uint32_t tgt;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { tgt = g_nextStepTargetMs; }
    int32_t deltaDecis = ((int32_t)tgt - (int32_t)g_elapsedMs) / 100;
    if (deltaDecis >= 1 && deltaDecis <= 3) emitSound(TONE_UP, 50);
  }
}
```

---

## 6. Armi del timer (start countdown/focus/teststrip)

Sostituisce `appTimer.every(100, …)` attuale.

```cpp
// include/functions/timer_manager.h

inline void startCountdownTimer() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_remainingMs = (uint32_t)appoTimeSet * 100UL;
    g_elapsedMs = 0;
    g_relayGate = channelsRelayActivated[channel - 1];
    g_modeActive = 2;
  }
  if (g_relayGate) RELAY_ON();
  isActionStarted = true;
}

inline void startFocusTimer() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_elapsedMs = 0;
    g_remainingMs = 0;
    g_relayGate = channelsRelayActivated[channel - 1];
    g_modeActive = 1;
  }
  if (g_relayGate) RELAY_ON();
  isActionStarted = true;
}

inline void startTestStripTimer() {
  if (!isActionStarted && g_elapsedMs == 0) {
    // primo avvio: inizializza target
    uint32_t firstTarget = (appoTimeSet > 0 ? appoTimeSet : 10) * 100UL;
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
      g_nextStepTargetMs = firstTarget;
    }
  }
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    g_relayGate = channelsRelayActivated[channel - 1];
    g_modeActive = 3;
  }
  if (g_relayGate) RELAY_ON();
  isActionStarted = true;
}

inline void stopAllTimers() {
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { g_modeActive = 0; }
  RELAY_OFF();
  isActionStarted = false;
  emitStopSound();
  uint32_t val;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
    val = (g_modeActive == 2) ? g_remainingMs / 100 : g_elapsedMs / 100;
  }
  showTime((int)val);
}
```

---

## 7. Sound state machine rebased

Sostituzione `millis()` → `g_tickMs`. Lettura atomica dal main.

```cpp
// include/functions/sound.h  (diff concettuale)

uint32_t readTickMs() {
  uint32_t t;
  ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { t = g_tickMs; }
  return t;
}

// Sostituire tutte le ricorrenze di millis() con readTickMs().
// Stessa logica, stesso layout state machine, solo sorgente temporale.
```

---

## 8. PROGMEM migration (Fase E)

### MODES_TEXTS in PROGMEM

```cpp
// include/constants.h  (riscritto)
#include <avr/pgmspace.h>

const char MODES_TEXT_0[] PROGMEM = "FOCUS    ";
const char MODES_TEXT_1[] PROGMEM = "COUNTDOWN";
const char MODES_TEXT_2[] PROGMEM = "TEST STRIP";
const char* const MODES_TEXTS[] PROGMEM = {
  MODES_TEXT_0, MODES_TEXT_1, MODES_TEXT_2
};

const char CHANNELS_STR_0[] = "Ch:A";  // 4 char, in RAM, trascurabile
const char CHANNELS_STR_1[] = "Ch:B";
const char CHANNELS_STR_2[] = "Ch:C";
const char* const CHANNELS_STR[] = {
  CHANNELS_STR_0, CHANNELS_STR_1, CHANNELS_STR_2
};
```

### Lettura PROGMEM nei render

```cpp
// include/functions/presentation.h
void showModeName(uint8_t modeId) {
  char buf[12];
  char* p = (char*)pgm_read_ptr(&MODES_TEXTS[modeId]);
  strcpy_P(buf, p);
  writePosition(0, 0, buf);
}
```

### types.h riscritto (estratto)

```cpp
struct PrecisionValueString {
  uint8_t     value;
  const char* str;          // letterale short (≤3 char), in RAM
};

struct ModalityTypeStruct {
  ModalityType id;
  const char*  value;
  constexpr ModalityTypeStruct(ModalityType i, const char* v) : id(i), value(v) {}
};

struct PrecisionTypeStruct {
  PrecisionType id;
  const char*   value;
  constexpr PrecisionTypeStruct(PrecisionType i, const char* v) : id(i), value(v) {}
};

struct Operation {
  void (*showPresentation)();
  void (*switchModality)();
  void (*switchPrecisionType)();
  void (*startedAction)();
  OperationStatusDetail status;
  const char* name;         // era String
  // ... costruttori con const char* invece di String
};
```

### Array precisioni

```cpp
// RAM diretta (11 × 2 puntatori × 2 byte = 44 B — trascurabile)
const PrecisionValueString FSTOP_PRECISIONS[PRECISION_COUNT] = {
  {1, "1"}, {2, "1/2"}, {3, "1/3"}, {4, "1/4"}, {6, "1/6"},
  {8, "1/8"}, {12, "1/12"}, {16, "1/16"}, {24, "1/24"},
  {32, "1/32"}, {64, "1/64"}
};
const PrecisionValueString SECONDS_PRECISIONS[PRECISION_COUNT] = {
  {1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}, {5, "5"},
  {6, "6"}, {7, "7"}, {8, "8"}, {10, "10"}, {15, "15"}, {20, "20"}
};
```

### getOperationByName — sostituzione

```cpp
// Attuale: matching su String, usato in initializeOperations().
// Nuovo: switch su MODES_IDS (indice), O(1).

Operation getOperationById(uint8_t id) {
  return operations[id];  // array globale, pre-popolato
}
```

---

## 9. Diff per file — riassunto

### Fase A

**Nuovo:** `include/functions/timer_isr.h`
- `setupTimer1()`
- `ISR(TIMER1_COMPA_vect)` (versione minima: solo `g_tickMs++` e `g_tickFlag100ms`)
- Variabili `volatile` comuni

**Modificato:** `include/variables.h`
- Rimosso `extern Timer<3> appTimer;`
- Aggiunte variabili `volatile` del motore temporale

**Modificato:** `src/main.cpp`
- Rimosso `Timer<3> appTimer;`
- Rimosso `appTimer.tick();` in loop
- Aggiunto `#include "functions/timer_isr.h"` in posizione corretta (dopo `variables.h`, prima di `base.h`)
- Aggiunto `setupTimer1()` in `setup()`
- Loop ristrutturato (vedi § 5)

**Modificato:** `include/functions/timer_manager.h`
- Sostituisce tutti i callback `arduino-timer` con handler `on*()`
- Sostituisce tutte le `appTimer.every(100, cb)` con start`<mode>`Timer
- Rimosso `appTimer.cancel()`

**Modificato:** `platformio.ini`
- Rimosso `contrem/arduino-timer@^3.0.1`

### Fase B

**Modificato:** `include/functions/base.h`
- `relayStart(bool on)` ora usa `RELAY_ON()`/`RELAY_OFF()` macro

**Modificato:** `include/functions/timer_isr.h`
- ISR estesa con logica completa COUNTDOWN e flip relè (vedi § 4)

### Fase C

**Modificato:** `include/functions/sound.h`
- Wrapper `readTickMs()` con `ATOMIC_BLOCK`
- Sostituzione sistematica `millis()` → `readTickMs()`

### Fase D

**Modificato:** `include/functions/timer_isr.h`
- Aggiunta logica `g_metronomeFlag` e `g_nextStepReached`

**Modificato:** `src/main.cpp`
- Consumo dei nuovi flag in loop (`onMetronomeBeat()`, `onTestStripStep()`)

**Modificato:** `include/functions/timer_manager.h`
- `onMetronomeBeat()` come handler main-side

### Fase E

**Modificato:** `include/types.h`
- `String` → `const char*` in tutti gli struct
- Costruttori aggiornati

**Modificato:** `include/constants.h`
- `MODES_TEXTS` in PROGMEM
- `CHANNELS_STR` come `const char*`

**Modificato:** `include/functions/presentation.h`
- Firme `writePosition/writeRow` accettano `const char*`
- `showModeName` usa `strcpy_P` per leggere da PROGMEM
- Rimossi tutti i `String(...)` wrapper

**Modificato:** `include/functions/operations.h`
- `Operation::name` è `const char*`
- `getOperationByName` sostituita con `getOperationById` (o eliminata)

**Modificato:** `platformio.ini`
- Rimosso `mike-matera/ArduinoSTL@^1.3.3`

### Fase F

- Esclusivamente verifica e test manuale. Nessun diff di codice atteso.

---

## 10. Pattern `ATOMIC_BLOCK` — quando e perché

AVR 8-bit: letture/scritture di variabili > 8 bit non sono atomiche. Se un'ISR scatta durante un accesso multi-byte, la lettura è corrotta.

### Regola

Ogni lettura/scrittura dal main di:
- `g_tickMs` (32-bit)
- `g_remainingMs` (32-bit)
- `g_elapsedMs` (32-bit)
- `g_nextStepTargetMs` (32-bit)

deve essere in `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`.

### Esenzioni

Flag a 8 bit (`g_tickFlag100ms`, `g_endOfCountdown`, `g_metronomeFlag`, `g_nextStepReached`, `g_modeActive`, `g_relayGate`): letture/scritture atomiche per natura, nessun `ATOMIC_BLOCK` necessario. Tuttavia il **clear-after-read** in loop richiede comunque `ATOMIC_BLOCK` perché è una read-modify-write (altrimenti un flag alzato dall'ISR tra la read e il clear sarebbe perso).

### Overhead

`ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` disabilita interrupt per la durata del blocco (tipicamente ≤ 20 cicli). Su 16 MHz è ~1.25 µs, accettabile.

---

## 11. Contabilità risorse attesa

### RAM (8192 B totali)

| Voce | Baseline | Post-refactor | Δ |
|---|---|---|---|
| ArduinoSTL globals | +150 B | 0 | −150 |
| `String` objects + buffers | +400 B | 0 | −400 |
| `Timer<3>` arduino-timer | +40 B | 0 | −40 |
| Nuove variabili `volatile` | 0 | +30 B | +30 |
| **Totale delta** | | | **−560 B** |
| **Uso stimato** | 2050 B | ~1490 B | **18.2%** |

### Flash (253952 B totali)

| Voce | Baseline | Post-refactor | Δ |
|---|---|---|---|
| ArduinoSTL code | +4000 B | 0 | −4000 |
| `String` methods | +1500 B | 0 | −1500 |
| arduino-timer library | +600 B | 0 | −600 |
| Nuovo ISR + setup | 0 | +400 B | +400 |
| **Totale delta** | | | **−5700 B** |
| **Uso stimato** | 28082 B | ~22382 B | **8.8%** |

Conforme a REQ-4.1 (≤20%) e REQ-4.2 (≤15%) con margine.

---

## 12. Procedure di validazione

### V1 — Cronometro esposizione (REQ-1.3)

Setup: cronometro digitale al millisecondo; relè pin 47 connesso a oscilloscopio o logic analyzer (o a un LED con fotodiodo + scope).

Procedura:
1. Imposta timeSet = 100 decis (10.0 s) in canale A con relè attivato
2. Premi main button → misura tempo tra flip ON e flip OFF del pin 47
3. Ripeti per 10.0 s, 30.0 s, 60.0 s (timeSet = 100, 300, 600)
4. Ripeti 10 volte per ogni durata

Pass: tutti i campioni entro 2 ms dall'aspettativa teorica.

### V2 — Jitter metronomo (REQ-1.5)

Setup: microfono vicino al buzzer, registrazione Audacity 44.1 kHz.

Procedura:
1. Avvia COUNTDOWN con timeSet = 300 decis (30 s)
2. Registra audio per tutti i 30 secondi
3. In Audacity: misura intervallo temporale tra i picchi dei 30 tick del metronomo
4. Calcola deviazione standard degli intervalli

Pass: σ < 1 ms su 29 intervalli.

### V3 — Test-strip step beep (REQ-1.4)

Setup: come V2.

Procedura:
1. TEST_STRIP, CONTINOUS, SECONDS, precisione 2 s
2. Avvia con target iniziale 10
3. Il beep di step avviene a t=10s, 12s, 14s, 16s, ...
4. Misura latenza tra tick teorico e picco beep in Audacity

Pass: latenza < 1 ms rispetto al tick teorico (clock interno), consistente tra i campioni.

### V4 — Consistenza funzionale (REQ-3.*)

Setup: checklist manuale con `DOCS.md` / `CLAUDE.md §11-16`.

Procedura: ogni comportamento listato viene eseguito sul firmware nuovo e confrontato con il comportamento del branch `Libraries_Refactoring` installato temporaneamente per confronto diretto.

Pass: nessuna differenza osservabile.

### V5 — Risorse (REQ-4.1, 4.2)

Output di `pio run` dopo Fase F:

```
RAM:   [=====     ]   XX.X% (used XXXX bytes from 8192 bytes)
Flash: [=         ]    X.X% (used XXXXX bytes from 253952 bytes)
```

Pass: RAM ≤ 20%, Flash ≤ 15%.

---

## 13. Rollback

Ogni fase = singolo commit. Rollback:

```bash
git checkout Performance_Rewrite
git log --oneline                    # identifica commit della fase problematica
git reset --hard <commit-prima>      # torna allo stato precedente
```

Rollback totale:

```bash
git checkout Libraries_Refactoring   # branch intatto, produzione invariata
git branch -D Performance_Rewrite    # (opzionale) elimina branch abbandonato
```

---

## 14. Cose da verificare in fase di plan

Dettagli che emergeranno solo leggendo il codice attuale durante l'implementazione:

1. **`focusTickCB` esistente usa `timerCurrentValue` in decisecondi** — va mappato su `g_elapsedMs / 100` nella nuova logica. Verificare che nessuna altra parte del codice legga direttamente `timerCurrentValue` come variabile globale.
2. **Metronomo attuale in callback 100 ms** — dopo la rimozione, verificare che non ci siano side-effect (es. reset di altri stati) incorporati nel callback.
3. **Logica di warning pre-step test-strip** (3 decis pre-target) — preservata nel nuovo `onTick100ms`, ma la finestra di trigger cambia da "cercata nel tick 100ms" a "cercata sempre nel tick 100ms confrontando con target esatto in ms". Validare che l'esperienza utente resti identica.
4. **`SoftwareSerial` e interrupt di Timer1** — verificare empiricamente che `TIMER1_COMPA_vect` sia servita durante una trasmissione `SoftwareSerial`. L'Arduino core assegna priorità alle ISR basata sul vector number; Timer1 ha priorità più alta di `PCINT` (usati da SoftwareSerial). Dovrebbe funzionare, ma misurare.
5. **Comportamento `tone()` interferenze con Timer1** — `tone()` usa Timer2, non Timer1. Nessuna interferenza attesa. Verificare.

---

## 15. Note finali

Il design preserva integralmente il **contratto utente** del firmware (comportamenti descritti in `DOCS.md`) modificando esclusivamente la **rappresentazione interna del tempo** (da decisecondi via `millis()` polled a millisecondi via Timer1 ISR hw).

La retrocompatibilità EEPROM è garantita: il layout `0xA5`-prefissato rimane identico, `timeSet`/`channelTime[]` continuano a essere serializzati in decisecondi.

Il modello di compilazione one-TU con header-everything, documentato come load-bearing in `CLAUDE.md §5`, viene preservato. L'unico nuovo header (`timer_isr.h`) segue le stesse convenzioni degli altri file `include/functions/*.h`.
