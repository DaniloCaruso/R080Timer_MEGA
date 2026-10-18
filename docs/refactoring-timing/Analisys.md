# Analisys — Refactoring Timing R080 Timer MEGA

**Documento:** Sintesi del processo di brainstorming e decisioni prese
**Data sessione:** 2026-04-18
**Branch corrente:** `Libraries_Refactoring`
**Branch target:** `Performance_Rewrite`

---

## 1. Richiesta originale

L'utente ha richiesto un'analisi del progetto R080 Timer MEGA volta a valutare la possibilità di una riscrittura che mantenga **le stesse logiche funzionali** ma con **performance significativamente superiori**. Requisito qualitativo: *"questo timer non deve perdere un colpo mai deve essere perfetto dal punto di vista del timing near zero"*.

Contesto del progetto: firmware per timer di ingranditore fotografico da camera oscura, Arduino Mega 2560, PlatformIO, branch `Libraries_Refactoring` con refactoring architetturale recente già completato.

---

## 2. Analisi dello stato attuale

Esplorata la struttura tramite `CLAUDE.md` (documentazione esaustiva), `src/main.cpp`, `platformio.ini` e gli header in `include/functions/`.

### Elementi che impattano il timing

| Elemento | Impatto stimato | Natura |
|---|---|---|
| `contrem/arduino-timer` | Polling su `millis()` via `appTimer.tick()`; jitter = worst-case `loop()` | Libreria |
| `SoftwareSerial` verso 7-seg @ 9600 baud | ~5 ms di trasmissione bloccante per update (interrupt disabilitati) | HAL scelta |
| `Wire`/I²C verso LCD Waveshare | 1-2 ms per update | Hardware bus |
| Metronomo `arTime % 10 == 0` in callback 100 ms | Jitter = jitter callback | Logica |
| Completamento countdown dentro callback polled | Relè OFF con precisione = polling | Logica |

### Elementi che impattano le risorse

| Elemento | Impatto |
|---|---|
| `mike-matera/ArduinoSTL` | ~3-5 KB flash, ~100-300 B RAM (globali inizializzazione) |
| Uso pervasivo di `String` | 30+ istanze; ~300-600 B RAM tra oggetti e heap |

### Baseline risorse (da CLAUDE.md §17)

- **RAM:** 2050 / 8192 B (25.0%)
- **Flash:** 28082 / 253952 B (11.1%)

---

## 3. Processo di brainstorming

### Domanda 1 — Motivazione

*Cosa motiva la richiesta?*

Opzioni: (a) jitter misurato, (b) preventivo, (c) nuove feature, (d) altro.

**Risposta utente: (b) preventivo**

Implicazione: abbiamo margine di progettazione, senza pressione di un bug in produzione.

### Domanda 2 — Target di precisione

*Quale ordine di grandezza di precisione?*

Opzioni:
- (a) ±10 ms — ottimizzazioni locali al `loop()`
- (b) ±1 ms — Timer1 CTC ISR hw + relè nell'ISR
- (c) ±100 µs — anche spostamento SoftwareSerial → Serial3 hw
- (d) ±10 µs — registri AVR puri, eliminazione HAL Arduino

**Risposta utente: (b) ±1 ms**

Razionale: per esposizione fotografica 1 ms è ampiamente sotto la soglia di percezione (0.1% di errore su 1 secondo). (c) e (d) non portano benefici pratici proporzionati al costo.

### Domanda 3 — Ampiezza dell'intervento

*Quanto codice possiamo toccare?*

Opzioni:
- (a) Chirurgico — solo sostituzione `arduino-timer`
- (b) Moderato — (a) + rimozione `ArduinoSTL`/`String`
- (c) Riscrittura completa — nuova architettura `.cpp`/`.h`, HAL astratta, testabilità host

**Risposta utente: (b) moderato**

Razionale: obiettivo è performance + timing, non manutenibilità/testabilità. Il modello one-TU header-everything resta.

---

## 4. Architettura proposta

### Motore temporale

Sostituzione di `arduino-timer` con un **Timer1 in modalità CTC a 1 kHz**. L'ISR (`ISR(TIMER1_COMPA_vect)`) è il cuore temporale: incrementa un contatore monotonico `g_tickMs`, gestisce il decremento del tempo residuo nel COUNTDOWN, e alza flag (`g_tickFlag100ms`, `g_endOfCountdown`, `g_nextStepReached`, `g_metronomeFlag`) che il main loop consuma al giro successivo.

### Commutazione relè nell'ISR

Quando `g_remainingMs` raggiunge zero durante un COUNTDOWN, l'ISR commuta direttamente il pin 47 via `PORTL &= ~(1 << PL2)`. Questo dà precisione al microsecondo sul bordo di fine esposizione, indipendente da cosa stia facendo il main loop (incluso update di SoftwareSerial in corso).

### Divisione ISR ↔ Main loop

**ISR (<20 µs):** decisioni temporali, manipolazione PORT, alzata di flag.
**Main loop:** display (LCD + 7-seg), sequenze audio complesse, EEPROM, input utente.

Il main loop consuma i flag alzati dall'ISR in modo event-driven, garantendo latenza <1 ms tra evento hw e reazione visibile/udibile.

### Sound state machine rebased

`SoundPlayer` cambia sorgente temporale da `millis()` a `g_tickMs`. Stessa logica, clock coerente con il resto del sistema, tick metronomo perfettamente in fase con il contatore hardware.

### Rimozione `ArduinoSTL` + `String`

Sostituzione di `String` con `const char*` (letterali o PROGMEM via `strcpy_P`) in:
- `types.h` (Operation::name, PRECISIONS[].string, ModalityTypeStruct, PrecisionTypeStruct)
- `constants.h` (CHANNELS_STR, MODES_TEXTS)
- `operations.h` (firma `getOperationByName`, probabilmente eliminabile via switch su `MODES_IDS`)
- `presentation.h` (firme `writePosition`/`writeRow`)

Guadagno stimato: **~500 B RAM + ~4 KB flash**.

---

## 5. Piano in 6 fasi

Ogni fase = commit separato, ogni fase produce un sistema funzionante.

| Fase | Contenuto | Verifica |
|---|---|---|
| **A** | Setup Timer1 ISR + scheletro flag + handler 100ms per tutti i modi; rimozione `arduino-timer` | Blink LED di debug; cronometro su countdown |
| **B** | Relay OFF dentro ISR via `PORTL` | Cronometro esterno su esposizione 10.0 s |
| **C** | Sound state machine rebased su `g_tickMs` | Ascolto metronomo |
| **D** | Metronomo e step beep triggered da flag ISR-set | Audacity per std-dev periodo tick |
| **E** | Rimozione `String`/`ArduinoSTL` + PROGMEM | Confronto dimensioni `pio run` |
| **F** | Validazione end-to-end; rilettura DOCS.md | Test manuale completo su hardware |

---

## 6. Criteri di successo

1. **Esposizione:** errore ≤ 2 ms su cronometro esterno per durate fino a 60 s
2. **Metronomo:** deviazione standard periodo tick < 1 ms su 30 s (Audacity)
3. **RAM:** ≤ 20% (target ~1600/8192 B)
4. **Flash:** ≤ 15%
5. **Funzionale:** zero regressioni rispetto a `DOCS.md` (tutti i comportamenti UI identici)
6. **Build:** nessun warning nuovo, nessun `.cpp` aggiunto

---

## 7. Rischi identificati e mitigazioni

| # | Rischio | Probabilità | Mitigazione |
|---|---|---|---|
| R1 | Contesa Timer1 con altra lib | Trascurabile | Timer0=millis, Timer2=tone verificati; primo commit fase A valida con LED blink |
| R2 | SoftwareSerial blocca 5 ms e desincronizza metronomo | Media | Update 7-seg schedulato fuori dalla finestra ±5 ms del tick metronomo |
| R3 | Race condition su `volatile` multi-byte | Media | `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` per letture dal main |
| R4 | Relè spike durante ISR | Nulla | Flip PORT è atomico 1-istruzione |
| R5 | `ArduinoSTL` rimossa rompe qualcosa di transitivo | Bassa | Se fallisce il build, ripristino selettivo della lib; metà del guadagno RAM resta dalla rimozione `String` dai nostri file |

---

## 8. Cosa NON verrà toccato

Perimetro deliberatamente lasciato fuori scope per contenere il rischio:

- Librerie `paulstoffregen/Encoder`, `chris--a/Keypad`, `EncoderButton`, `OneButton`
- Layout EEPROM (magic `0xA5`, 13 byte)
- Pinout hardware (nessun filo spostato)
- Semantica UI (stringhe italiane, layout display, keypad actions, F-Stop formula)
- Modello compilazione one-TU con header-everything

---

## 9. Follow-up opzionali (non in questo scope)

Idee emerse durante il brainstorming, rimandate:

- **7-seg su Serial3 hw** (TX pin 14 libero) per eliminare del tutto i 5 ms bloccanti di SoftwareSerial. Richiede spostamento di un filo e modifiche al driver del 7-seg. Valutabile come fase G futura se REQ-6.2 risulta difficile da rispettare.
- **Testabilità host** (mock di `millis()`/PORT) — richiederebbe il modello (c) scartato.
- **Sostituzione `OneButton`/`EncoderButton`** con codice custom più leggero — non impatta il path critico, solo cosmetica di RAM/flash.

---

## 10. Stato documenti

Questo documento (`Analisys.md`) è accompagnato in `docs/refactoring-timing/` da:

- **`EARS.md`** — Requisiti formali in notazione EARS (tracciabilità a fasi)
- **`Technical.md`** — Design tecnico dettagliato (codice, registri, diff per file)
- **`Management.md`** — Sommario esecutivo per decisione go/no-go

Il piano di implementazione dettagliato (task list eseguibile) sarà prodotto come fase successiva, dopo approvazione di questi documenti.
