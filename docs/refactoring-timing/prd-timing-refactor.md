# PRD: Refactoring Timing R080 Timer MEGA — Motore Temporale Hardware ISR

## Overview

Rifacimento mirato del motore temporale del firmware R080 Timer MEGA per ottenere precisione garantita di ±1 ms sul bordo di fine esposizione fotografica e deviazione standard <1 ms sul periodo del metronomo. La sostituzione centrale è: `arduino-timer` (polling software su `millis()`) → **Timer1 CTC ISR hardware a 1 kHz** (AVR ATmega2560). Nello stesso intervento vengono rimosse le librerie `ArduinoSTL` e `contrem/arduino-timer` e la classe `String` viene sostituita con stringhe `const char*` (letterali o PROGMEM).

Contesto di progetto critico per l'agente: il firmware compila come **singola unità di traduzione** con `src/main.cpp` unico `.cpp`; tutti gli header in `include/functions/*.h` **definiscono** funzioni inline. **Non** creare nuovi `.cpp`. Leggere `CLAUDE.md` e i 4 documenti in `docs/refactoring-timing/` (EARS.md, Analisys.md, Technical.md, Management.md) prima di ogni story.

## Goals

- Precisione ±1 ms sul flip del relè a fine esposizione (REQ-1.2)
- Deviazione standard periodo metronomo < 1 ms su 30 s di registrazione (REQ-1.5)
- Latenza step beep test-strip < 1 ms (REQ-1.4)
- RAM occupata ≤ 20% (target ~1490 / 8192 byte, da 25.0%)
- Flash occupata ≤ 15% (target ~22400 / 253952 byte)
- Zero regressioni funzionali rispetto a `DOCS.md` / `CLAUDE.md §11-16`
- Modello one-TU header-everything preservato
- Layout EEPROM invariato (magic `0xA5`, 13 byte)

## Quality Gates

Questi comandi e verifiche devono passare per OGNI user story prima di marcarla completata:

1. **`pio run`** — compila con exit 0 e **zero warning nuovi** rispetto alla baseline `Libraries_Refactoring`
2. **Checklist manuale hardware** — l'agente produce una `VERIFY.md` con i passi di verifica specifici della story; l'operatore umano li esegue sul timer fisico e marca pass/fail

Per le stories che introducono codice ISR o modificano il path critico del tempo, includere anche:

3. **Test cronometrico esterno** — misura dell'esposizione con cronometro digitale per almeno 3 durate (10.0 s, 30.0 s, 60.0 s), errore max 2 ms

Per la story di validazione finale (US-006), includere anche:

4. **Test Audacity** — registrazione di 30 s di metronomo, calcolo σ degli intervalli tick < 1 ms

**Branch di lavoro:** tutto il lavoro su `Performance_Rewrite` (da `Libraries_Refactoring`). Il branch base resta intatto come fallback. **L'agente non esegue mai comandi git mutanti** (nessun `git add`/`commit`/`push`/`reset`); lascia il working tree sporco e l'operatore umano committa.

## User Stories

### US-001: Fase A — Motore temporale Timer1 ISR

**Description:** Come manutentore del firmware, voglio sostituire `contrem/arduino-timer` (polling su `millis()`) con un Timer1 hardware in modalità CTC che genera un interrupt a 1 kHz, così da avere un clock deterministico immune al worst-case del main loop.

**Acceptance Criteria:**
- [ ] Creato nuovo header `include/functions/timer_isr.h` con `#ifndef`/`#define`/`#endif` guards, funzione `setupTimer1()` e `ISR(TIMER1_COMPA_vect)`
- [ ] Registri configurati: `TCCR1A=0`, `TCCR1B` con `WGM12` + `CS11` + `CS10` (prescaler 64), `OCR1A=249`, `TIMSK1 |= (1<<OCIE1A)`
- [ ] Variabili `volatile` aggiunte in `include/variables.h`: `g_tickMs` (uint32_t), `g_remainingMs` (uint32_t), `g_elapsedMs` (uint32_t), `g_nextStepTargetMs` (uint32_t), `g_relayGate` (uint8_t), `g_modeActive` (uint8_t), `g_tickFlag100ms` (uint8_t), `g_endOfCountdown` (uint8_t), `g_nextStepReached` (uint8_t), `g_metronomeFlag` (uint8_t)
- [ ] Rimosso `extern Timer<3> appTimer;` da `variables.h`
- [ ] Rimosso `Timer<3> appTimer;` e `appTimer.tick();` da `src/main.cpp`
- [ ] Aggiunto `#include "functions/timer_isr.h"` in `src/main.cpp` dopo `variables.h` e prima di `base.h`
- [ ] Aggiunta chiamata `setupTimer1()` in `setup()` come ultima istruzione prima del banner `PRONTO`
- [ ] `loop()` ristrutturato con lettura atomica dei flag via `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` e chiamata degli handler `onTick100ms`/`onCountdownEnd`/`onTestStripStep`/`onMetronomeBeat`
- [ ] `include/functions/timer_manager.h` riscritto: handler main-side `onTick100ms`, `onCountdownEnd`, `onTestStripStep`, `onMetronomeBeat`; start functions `startFocusTimer`/`startCountdownTimer`/`startTestStripTimer`/`stopAllTimers` (senza più `appTimer.every`)
- [ ] In `platformio.ini` rimossa la riga `contrem/arduino-timer@^3.0.1`
- [ ] Header load-bearing include order in `src/main.cpp` preservato (documentato in CLAUDE.md §5)
- [ ] Nessun nuovo `.cpp` aggiunto al progetto
- [ ] Prodotto `VERIFY.md` con checklist hardware:
  - Boot banner "=== R080 TIMER PRONTO ===" appare su seriale
  - FOCUS: timer sale 0→10.0→20.0 con incremento di 0.1 ogni 100 ms
  - COUNTDOWN: da 10.0 s scende a 0 in ~10 s (cronometro esterno, errore ≤ 100 ms per questa fase)
  - TEST_STRIP CONTINOUS: timer sale come FOCUS
  - Tutti i 3 canali switchabili con A/B/C, tempi memorizzati
  - Encoder modo e precisione reattivi quando stopped
  - Keypad digit entry funzionante, mute/hold # funzionanti

### US-002: Fase B — Commutazione relè dentro l'ISR

**Description:** Come fotografo di camera oscura, voglio che il relè dell'ingranditore si apra con precisione al microsecondo sul bordo di fine esposizione, così da avere stampe con esposizione esatta, indipendente da cosa stia facendo il display in quel momento.

**Acceptance Criteria:**
- [ ] Aggiunte macro in `include/functions/timer_isr.h` (o in `base.h`): `#define RELAY_PORT PORTL`, `#define RELAY_BIT PL2`, `#define RELAY_ON()  (RELAY_PORT |= (1 << RELAY_BIT))`, `#define RELAY_OFF() (RELAY_PORT &= ~(1 << RELAY_BIT))`
- [ ] Pin 47 confermato come PL2 (verifica da datasheet ATmega2560 §13.4 oppure table pinout Arduino Mega)
- [ ] `relayStart(bool on)` in `include/functions/base.h` usa le macro `RELAY_ON()` / `RELAY_OFF()` al posto di `digitalWrite(ENLARGER_RELAY_PIN, …)`
- [ ] ISR `TIMER1_COMPA_vect` estesa: quando `g_modeActive == 2` (COUNTDOWN) e `g_remainingMs == 0`, esegue `if (g_relayGate) RELAY_OFF();` e alza `g_endOfCountdown = 1`
- [ ] `startCountdownTimer()` copia lo stato del gate canale in `g_relayGate` dentro `ATOMIC_BLOCK`: `g_relayGate = channelsRelayActivated[channel - 1];`
- [ ] ISR resta sotto 20 µs worst-case (verifica empirica: nessun `delay`, nessun I/O, solo PORT + flag)
- [ ] Prodotto `VERIFY.md` con checklist hardware:
  - Canale A attivato (⚡ visibile LCD), COUNTDOWN 10.0 s, cronometro start al main button press, stop al click del relè udibile
  - Errore ≤ 2 ms su 5 tentativi per durate 10.0, 30.0, 60.0 s
  - Canale con ⚡ disattivato (hold #): timer gira ma relè resta sempre OFF (verifica con LED o oscilloscopio su pin 47)
  - Repeat-last-exposure: a fine countdown `timeSet` torna al valore iniziale (verificare su 7-seg)
  - Stop manuale (secondo click main button) interrompe esposizione, relè OFF immediato

### US-003: Fase C — SoundPlayer rebased su clock hardware unificato

**Description:** Come manutentore del firmware, voglio che la state machine audio usi `g_tickMs` (clock Timer1 ISR) invece di `millis()` (clock Timer0 Arduino), così da avere un'unica sorgente temporale coerente in tutto il sistema e eliminare drift tra sottosistemi.

**Acceptance Criteria:**
- [ ] Aggiunta helper `readTickMs()` (inline) in `include/functions/timer_isr.h` o `sound.h`: lettura di `g_tickMs` dentro `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`, ritorna `uint32_t`
- [ ] In `include/functions/sound.h`, tutte le occorrenze di `millis()` sostituite con `readTickMs()`
- [ ] La state machine `SoundPlayer` preserva identici: sequenza stop (TONE_DOWN 75/150p/75/150p/150), durate beep (metronome 80 ms, test-strip warning 50 ms, step 150 ms, unmute 200 ms), comportamento mute (no-op)
- [ ] `emitStopSound()` continua a rifiutare sequenze sovrapposte (stato interno preservato)
- [ ] Nessuna regressione su suoni: metronomo, beep step, tono di conferma unmute, sequenza stop (identici a orecchio rispetto a `Libraries_Refactoring`)
- [ ] Prodotto `VERIFY.md` con checklist hardware:
  - COUNTDOWN 30 s: metronomo suona esattamente 30 volte (1/sec)
  - Stop manuale: sequenza stop completa (5 beep TONE_DOWN secondo pattern)
  - TEST_STRIP CONTINOUS SECONDS 2s: beep step al secondo 10, 12, 14, 16
  - TEST_STRIP warning beep (3 decis pre-step): udibili 3 beep brevi prima di ogni step
  - Toggle mute (hold *): beep di conferma solo sull'unmute (TONE_UP 200ms)

### US-004: Fase D — Metronomo e step beep trigger precisi ISR-driven

**Description:** Come fotografo, voglio che il metronomo suoni con precisione metronomica (tick esattamente al secondo) e che i beep di step test-strip siano sincronizzati al ms col tick teorico, così da poter contare i secondi ad orecchio con la stessa affidabilità di un metronomo meccanico.

**Acceptance Criteria:**
- [ ] ISR alza `g_metronomeFlag = 1` quando `g_modeActive == 2 && (g_remainingMs % 1000) == 0 && g_remainingMs > 0`
- [ ] ISR alza `g_nextStepReached = 1` quando `g_modeActive == 3 && g_elapsedMs >= g_nextStepTargetMs`
- [ ] Nuovo handler `onMetronomeBeat()` in `timer_manager.h`: `if (!mute) emitSound(TONE_DOWN, 80);`
- [ ] Nuovo handler `onTestStripStep()` in `timer_manager.h`: emette `emitSound(TONE_UP, 150)`, calcola prossimo target (FSTOP: `timerCurrentValue * 2^(1/precisionValue)` clamped; SECONDS: `+precisionValue*10` decis), aggiorna `g_nextStepTargetMs` dentro `ATOMIC_BLOCK`, applica logica STEP_BY_STEP (relay OFF + `isActionStarted=false`) se modalità lo richiede
- [ ] Rimossa la vecchia logica `arTime % 10 == 0` dal callback 100ms di countdown (non più necessaria)
- [ ] Rimossa la vecchia logica di check step in callback 100ms test-strip (sostituita dal flag ISR)
- [ ] Warning pre-step test-strip (`TONE_UP 50ms` quando delta target ∈ [1..3] decis) preservato dentro `onTick100ms`
- [ ] Resume STEP_BY_STEP funzionante: secondo click main button dopo step riparte dal `g_elapsedMs` corrente, non da zero
- [ ] Prodotto `VERIFY.md` con checklist hardware:
  - COUNTDOWN 30 s, registrazione audio con smartphone, apertura in Audacity o simile: misurare σ degli intervalli tra i 30 tick, deve essere < 1 ms (preferibile < 0.5 ms)
  - TEST_STRIP SECONDS precisione 2s continuo da 10: beep step udibili a 10, 12, 14, 16, 18 s misurati con cronometro esterno (errore < 1 ms dal secondo intero)
  - TEST_STRIP STEP_BY_STEP SECONDS 2s: dopo ogni step il timer si ferma, relè OFF, display mostra prossimo target; secondo click resume
  - TEST_STRIP FSTOP precisione 2 (1/2): beep step a tempi calcolati via formula `2^(0.5) ≈ 1.41`, verificare sequenza 10 → 14 → 20 → 28 (decis × 10 / arrotondato)

### US-005: Fase E — Rimozione String/ArduinoSTL + PROGMEM

**Description:** Come manutentore del firmware, voglio rimuovere le dipendenze `mike-matera/ArduinoSTL` e l'uso della classe `String` (sostituiti con `const char*` e PROGMEM dove appropriato), così da liberare ~500 B di RAM e ~4 KB di Flash senza sacrificare leggibilità.

**Acceptance Criteria:**
- [ ] In `include/types.h`: struct `PrecisionValueString`, `ModalityTypeStruct`, `PrecisionTypeStruct` usano `const char*` invece di `String`; costruttori aggiornati
- [ ] In `include/types.h`: struct `Operation` — campo `name` diventa `const char*`; tutti i costruttori aggiornati per accettare `const char*`
- [ ] Array `FSTOP_PRECISIONS` e `SECONDS_PRECISIONS` usano la nuova signature POD `{uint8_t value; const char* str;}`
- [ ] In `include/constants.h`: `MODES_TEXTS` in PROGMEM con `const char[] PROGMEM` per ogni stringa e `const char* const MODES_TEXTS[] PROGMEM`
- [ ] In `include/constants.h`: `CHANNELS_STR` come `const char*` (RAM, trascurabile)
- [ ] In `include/functions/presentation.h`: firme `writePosition(int, int, const char*)`, `writeRow(int, const char*)`; rimossi tutti i wrapper `String(...)`
- [ ] Accesso a `MODES_TEXTS` via `pgm_read_ptr` + `strcpy_P` in `keyBuffer` quando serve visualizzare
- [ ] In `include/functions/operations.h`: `getOperationByName(String)` sostituita con `getOperationById(uint8_t)` o eliminata se non più referenziata (confermare con grep)
- [ ] In `platformio.ini` rimossa la riga `mike-matera/ArduinoSTL@^1.3.3`
- [ ] Ogni occorrenza di `String` nel progetto verificata ed eliminata (grep globale `String` deve dare zero match nei file sorgente, esclusi commenti)
- [ ] Output di `pio run` mostra:
  - RAM ≤ 20% (target ~1490/8192)
  - Flash ≤ 15% (target ~22400/253952)
- [ ] Prodotto `VERIFY.md` con checklist hardware:
  - Boot regolare
  - Nome modi mostrato correttamente su LCD (FOCUS, COUNTDOWN, TEST STRIP) — verifica che lettura PROGMEM funzioni
  - Precisione FSTOP mostra "1/2", "1/3", ..., "1/64" ruotando encoder precisione
  - Precisione SECONDS mostra "1", "2", "3", ..., "20"
  - Indicatore canale mostra "Ch:A" / "Ch:B" / "Ch:C"
  - Nessuna stringa corrotta o caratteri sporchi sul display

### US-006: Fase F — Validazione end-to-end

**Description:** Come project owner, voglio una validazione sistematica di tutti i comportamenti documentati in `DOCS.md`/`DOCS-IT.md` più le misure strumentali dei criteri di successo, così da confermare il go/no-go del rifacimento.

**Acceptance Criteria:**
- [ ] Eseguita la checklist V1 (Cronometro esposizione) descritta in `Technical.md §12`:
  - Timer imposta 10.0 s, 30.0 s, 60.0 s in canale A
  - Misura con cronometro esterno (o scope su pin 47): 10 campioni per ogni durata
  - Tutti i campioni entro 2 ms dall'aspettativa teorica
  - Risultati documentati in `VERIFY.md`
- [ ] Eseguita la checklist V2 (Jitter metronomo) descritta in `Technical.md §12`:
  - COUNTDOWN 30 s, registrazione audio, apertura in Audacity
  - Misura σ degli intervalli tra tick: < 1 ms
  - Screenshot o export dati in `VERIFY.md`
- [ ] Eseguita la checklist V3 (Test-strip step beep):
  - TEST_STRIP CONTINOUS SECONDS precisione 2s, registrazione audio, analisi Audacity
  - Latenza beep < 1 ms rispetto al tick teorico
- [ ] Eseguita la checklist V4 (Consistenza funzionale): ogni sezione di `DOCS.md` (o `CLAUDE.md §11-16`) verificata manualmente:
  - FOCUS — keypad ignorato, `*`/`#` brightness, relè ON, timer sale
  - COUNTDOWN — metronomo, repeat-last-exposure, tono stop
  - TEST_STRIP — CONTINOUS e STEP_BY_STEP, FSTOP e SECONDS, warning pre-step
  - F-Stop `*`/`#` navigation, offset "+02"/"-01"/" 00"
  - Channel switch A/B/C, reset D, per-channel relay gate
  - Keypad HOLD (* mute, # channel relay)
  - EEPROM persistenza (spegni/riaccendi, settings preservati)
- [ ] Eseguita la checklist V5 (Risorse): output `pio run` allegato in `VERIFY.md`, conferma RAM ≤ 20% e Flash ≤ 15%
- [ ] Nessun warning aggiuntivo nel build rispetto alla baseline `Libraries_Refactoring`
- [ ] Nessun file `.cpp` aggiunto al progetto (conferma con `find src include -name '*.cpp'`)
- [ ] Ordine di include in `src/main.cpp` compatibile con CLAUDE.md §5
- [ ] Tutti i documenti `VERIFY.md` delle fasi A-E raccolti/referenziati
- [ ] Report finale go/no-go in `docs/refactoring-timing/GO-NO-GO.md` con:
  - Tabella criterio → valore misurato → pass/fail
  - Eventuali note su residui/follow-up
  - Raccomandazione merge a `main` o mantenere su `Performance_Rewrite`

## Functional Requirements

- **FR-1:** Il sistema deve utilizzare Timer1 hardware in modalità CTC con ISR a frequenza 1 kHz come unica sorgente di temporizzazione del sottosistema timer.
- **FR-2:** Quando `g_remainingMs` raggiunge zero durante un COUNTDOWN attivo, l'ISR deve commutare il pin 47 (PORTL bit 2) a livello LOW senza passare per `digitalWrite`.
- **FR-3:** Il sistema deve preservare l'unità di misura esterna "decisecondi" per display, keypad, EEPROM e rappresentazione canali; la conversione ms↔decis avviene solo ai confini (start timer / render).
- **FR-4:** Il sistema deve preservare il layout EEPROM esistente (magic `0xA5`, 13 byte) senza alterazioni.
- **FR-5:** Il sistema deve preservare il pinout hardware attuale (nessun filo spostato).
- **FR-6:** Il sistema deve eseguire tutte le letture multi-byte (uint32_t) di variabili `volatile` condivise dentro un blocco `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` dal main loop.
- **FR-7:** L'ISR deve completare in meno di 20 µs nel caso peggiore e non deve contenere chiamate a `delay`, I/O lento (LCD, SoftwareSerial), o funzioni che possano bloccare.
- **FR-8:** Il sistema deve rimuovere le dipendenze `mike-matera/ArduinoSTL` e `contrem/arduino-timer` dal `platformio.ini`.
- **FR-9:** Il sistema deve sostituire tutti gli usi della classe `String` nei file sorgente con `const char*` (letterali o PROGMEM).
- **FR-10:** Il sistema deve preservare il modello di compilazione a singola unità di traduzione: `src/main.cpp` resta l'unico file `.cpp` del progetto.
- **FR-11:** Ogni user story deve produrre un file `VERIFY.md` nella directory `docs/refactoring-timing/verify/` con nome `US-XXX-verify.md`, contenente la checklist hardware specifica della story e gli esiti.
- **FR-12:** L'agente non deve eseguire comandi git mutanti (`add`/`commit`/`push`/`reset`/`stash`/`checkout --`); il working tree deve restare sporco e la revisione/commit è responsabilità dell'operatore umano.

## Non-Goals (Out of Scope)

- Riscrittura architetturale completa (no `.cpp` aggiuntivi, no HAL astratta, no host test harness)
- Sostituzione delle librerie di input (`paulstoffregen/Encoder`, `chris--a/Keypad`, `mathertel/OneButton`, `stutchbury/EncoderButton`) — funzionano, non stanno nel path critico
- Spostamento fisico del 7-segmenti da SoftwareSerial (pin A2) a Serial3 (pin 14) — follow-up opzionale, non in questo refactoring
- Modifiche alla UI utente: layout LCD, testi italiani, semantica keypad, formula F-Stop
- Modifiche al layout EEPROM o al protocollo magic-byte
- Nuove funzionalità utente (focus esclusivo: performance e timing)
- Aggiornamento delle altre librerie a versioni più recenti

## Technical Considerations

- **Compilation model:** `src/main.cpp` è l'unico `.cpp`. Tutti i nuovi file sono header in `include/functions/` con guardie `#ifndef`/`#define`/`#endif` e funzioni definite inline. Creare un `.cpp` rompe il link per multiple-definition.
- **Include order:** documentato in `CLAUDE.md §5`. `timer_isr.h` va incluso dopo `variables.h` e prima di `base.h`.
- **Timer uso:** Timer0 è riservato ad Arduino core (`millis`, `delay`, `micros`); Timer2 è usato da `tone()`. Timer1 è libero e assegnato a questo refactoring. Timer3-5 restano liberi.
- **Pin mapping:** pin 47 su Arduino Mega = PL2 (PORTL bit 2). Verificare in datasheet ATmega2560 §13.4 prima del commit Fase B.
- **Volatile & atomicità:** AVR 8-bit non offre atomicità per accessi multi-byte. Usare `<util/atomic.h>` con `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` per ogni read-modify-write di `volatile` > 8-bit.
- **SoftwareSerial interaction:** durante una trasmissione SoftwareSerial (7-seg) gli interrupt sono disabilitati ~5 ms, ma Timer1 ha priorità vector più alta di PCINT (usato da SoftwareSerial), quindi l'ISR viene servita prima del completamento della trasmissione. Validare empiricamente in Fase A.
- **Git policy:** nessun comando git mutante. Vedi FR-12. L'agente deve essere configurato con `allowed_tools` che escludano `git add|commit|push|reset|stash|checkout`.

## Success Metrics

- **Precisione esposizione:** errore max 2 ms misurato su cronometro esterno per esposizioni fino a 60 s
- **Jitter metronomo:** σ periodo tick < 1 ms su registrazione 30 s (Audacity)
- **Latenza step beep:** < 1 ms rispetto al tick teorico
- **RAM:** ≤ 20% (target ~1490 / 8192 B, da baseline 2050 B)
- **Flash:** ≤ 15% (target ~22400 / 253952 B, da baseline 28082 B)
- **Zero regressioni:** checklist funzionale V4 completa passata
- **Build:** `pio run` exit 0 con zero warning aggiuntivi

## Open Questions

- In fase di plan, confermare via grep che `getOperationByName(String)` può essere eliminata (uso solo in `initializeOperations`?) o se deve essere convertita in `getOperationById(uint8_t)`. (Risolto prima di eseguire US-005.)
- Valutare in Fase A se la priorità interrupt di Timer1 effettivamente sovrasta SoftwareSerial su ATmega2560 (Timer1 vector #17, PCINT #3-5 — Timer1 ha priorità più alta, conferma empirica richiesta).
- Se il risultato misurato di V2 (jitter metronomo) eccede 1 ms nonostante l'ISR, aprire issue per fase G opzionale (spostamento 7-seg su Serial3 hw).

