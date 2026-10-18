# US-006 — Fase F — Validazione end-to-end e report go/no-go

## Branch / Commit

- Branch: `Libraries_Refactoring` (il piano originale prevedeva `Performance_Rewrite` come branch target; le Fasi A–F sono state implementate direttamente sul branch attivo, changes non committate per review manuale).
- Baseline di confronto: commit `85082a8 Refactor system architecture and remove complex dependencies` (stato pre-refactoring temporale).
- Stato: modifiche non commitate. Agente NON ha eseguito comandi git mutanti.

## Riferimento procedure

- `docs/refactoring-timing/Technical.md §12` (V1–V5)
- `docs/refactoring-timing/EARS.md` (REQ-1..REQ-8)
- Verify documenti precedenti: `US-001-verify.md`, `US-002-verify.md`, `US-003-verify.md`, `US-004-verify.md`, `US-005-verify.md` (stessa directory).

---

## Build finale (post Fase E, cumulativo A+B+C+D+E)

```
Processing megaatmega2560 (platform: atmelavr; board: megaatmega2560; framework: arduino)
--------------------------------------------------------------------------------
CONFIGURATION: https://docs.platformio.org/page/boards/atmelavr/megaatmega2560.html
PLATFORM: Atmel AVR (5.0.0) > Arduino Mega or Mega 2560 ATmega2560 (Mega 2560)
HARDWARE: ATMEGA2560 16MHz, 8KB RAM, 248KB Flash
PACKAGES:
 - framework-arduino-avr @ 5.2.0
 - toolchain-atmelavr @ 1.70300.191015 (7.3.0)
LDF Modes: Finder ~ chain, Compatibility ~ soft
Dependency Graph
|-- Encoder @ 1.4.4
|-- Keypad @ 3.1.1
|-- LiquidCrystal_I2C @ 1.1.4
|-- EncoderButton @ 1.0.6
|-- OneButton @ 2.6.1
|-- SoftwareSerial @ 1.0
|-- Wire @ 1.0
|-- WaveShare-Lib
Building in release mode
Checking size .pio/build/megaatmega2560/firmware.elf
RAM:   [==        ]  17.9% (used 1463 bytes from 8192 bytes)
Flash: [=         ]   9.7% (used 24564 bytes from 253952 bytes)
========================= [SUCCESS] Took 0.39 seconds =========================
```

**Exit code:** 0. **Warning nuovi vs. baseline `Libraries_Refactoring`:** 0 (il preesistente warning `ArduinoSTL limits header` è scomparso in Fase E con la rimozione della libreria; si tratta di miglioramento, non regressione).

### Storia delle misure (trend per fase)

| Fase | RAM (B / %) | Flash (B / %) | Δ RAM vs baseline | Δ Flash vs baseline |
|---|---|---|---|---|
| Baseline `Libraries_Refactoring` | 2050 / 25.0% | 28082 / 11.1% | — | — |
| A — Timer1 ISR | 2025 / 24.7% | 28014 / 11.0% | −25 | −68 |
| B — Relè in ISR | 2025 / 24.7% | 28152 / 11.1% | −25 | +70 |
| C — Sound su `readTickMs` | 2025 / 24.7% | 28216 / 11.1% | −25 | +134 |
| D — Metronomo+step ISR | 2025 / 24.7% | 28520 / 11.2% | −25 | +438 |
| **E — PROGMEM+no String** | **1463 / 17.9%** | **24564 / 9.7%** | **−587** | **−3518** |

---

## V5 — Risorse (REQ-4.1, REQ-4.2) — AUTOMATICO

- **Target REQ-4.1:** RAM ≤ 20% (≤ 1639 B / 8192 B).
- **Misurato:** 1463 B (17.9%). **Margine:** 176 B, 2.1 pt pct.
- [x] **PASS**

- **Target REQ-4.2:** Flash ≤ 15% (≤ 38093 B / 253952 B).
- **Misurato:** 24564 B (9.7%). **Margine:** 13529 B, 5.3 pt pct.
- [x] **PASS**

---

## Acceptance Criteria — verifica statica (agente)

### AC compilazione e modello one-TU

- [x] `pio run` compila con exit 0 e zero warning aggiuntivi rispetto alla baseline `Libraries_Refactoring` (il warning pre-esistente di `ArduinoSTL/limits` è stato ELIMINATO in Fase E, non è una regressione).
- [x] `find src include -name '*.cpp'` → solo `src/main.cpp` (nessun `.cpp` aggiunto in alcuna fase).
- [x] Ordine di include in `src/main.cpp` **compatibile** con CLAUDE.md §5 con le seguenti modifiche documentate e load-bearing:
  - Aggiunto `#include <util/atomic.h>` dopo `<avr/eeprom.h>` (necessario per `ATOMIC_BLOCK`, Fase A).
  - Rimosso `#include <arduino-timer.h>` (libreria rimossa, Fase A).
  - Aggiunto `#include "functions/timer_isr.h"` dopo `variables.h` e PRIMA di `base.h` (Fase A — vedi learnings progress.md).
  - Ordine finale verificato:
    ```
    1: Arduino.h
    2: Keypad.h
    3: avr/eeprom.h
    4: util/atomic.h
    5: Wire.h
    6: LiquidCrystal_I2C.h
    7: SoftwareSerial.h
    8: Encoder.h
    9: EncoderButton.h
    10: OneButton.h
    11-13: types.h, constants.h, variables.h
    14: functions/timer_isr.h
    15-24: functions/base.h, config.h, presentation.h, sound.h, storage.h, running_mgmt.h, operations.h, encoder_mgmt.h, keypad_mgmt.h, timer_manager.h
    ```
  - Conformità funzionale: tutti gli header `functions/*.h` vedono `g_tickMs` / `g_remainingMs` / `RELAY_ON`-`RELAY_OFF` / `readTickMs` grazie a questa insertion, come richiesto da CLAUDE.md §5.

### AC tracciabilità VERIFY fasi precedenti

- [x] US-001 (Fase A, Timer1 ISR) → `docs/refactoring-timing/verify/US-001-verify.md` (presente, build validato, checklist hardware lasciata all'operatore).
- [x] US-002 (Fase B, relè in ISR) → `docs/refactoring-timing/verify/US-002-verify.md`.
- [x] US-003 (Fase C, sound rebased) → `docs/refactoring-timing/verify/US-003-verify.md`.
- [x] US-004 (Fase D, metronomo+step ISR) → `docs/refactoring-timing/verify/US-004-verify.md`.
- [x] US-005 (Fase E, PROGMEM+no String) → `docs/refactoring-timing/verify/US-005-verify.md`.

### AC riguardanti git

- [x] L'agente NON ha eseguito comandi git mutanti (no commit, no reset, no checkout, no push). Tutte le modifiche rimangono non committate, per review manuale come da policy del workflow.

---

## V1 — Cronometro esposizione (REQ-1.2, REQ-1.3) — CHECKLIST OPERATORE

**Target REQ-1.3:** errore accumulato < 2 ms su esposizione fino a 60 s.
**Target REQ-1.2:** flip-off relè entro 1 ms dal bordo teorico in COUNTDOWN.

### Setup

- [ ] Cronometro digitale con risoluzione al ms **oppure** oscilloscopio/logic analyzer collegato al pin 47 (relè ingranditore) **oppure** LED + fotodiodo collegato a input analogico di un altro MCU con `micros()` calibrato.
- [ ] Firmware flashato con `pio run --target upload` sul branch in oggetto.
- [ ] Canale A con gate relè attivato (hold `#` finché icona ⚡ appare su col 15 riga 1).

### Procedura

Per ciascuna durata `T ∈ {100, 300, 600} decis` (= 10.0 s, 30.0 s, 60.0 s):

1. Imposta `timeSet = T` via keypad (es. `1 0 0` per 10.0 s, `3 0 0` per 30.0 s, `6 0 0` per 60.0 s).
2. Premi main button → registra timestamp `t_on` (millisecondo) del primo fronte HIGH su pin 47.
3. Attendi spegnimento relè → registra timestamp `t_off`.
4. Calcola `T_misurato = t_off - t_on` (ms), `errore = T_misurato - T*100` (ms).
5. Ripeti 10 volte per durata (stop e reset tra prove, main button per interrompere e ri-avviare).

### Risultati (compilare da operatore umano)

#### 10.0 s (teorico 10 000 ms)

| # | t_on (ms) | t_off (ms) | T_misurato (ms) | errore (ms) | pass (< 2 ms)? |
|---|---|---|---|---|---|
| 1  |  |  |  |  |  |
| 2  |  |  |  |  |  |
| 3  |  |  |  |  |  |
| 4  |  |  |  |  |  |
| 5  |  |  |  |  |  |
| 6  |  |  |  |  |  |
| 7  |  |  |  |  |  |
| 8  |  |  |  |  |  |
| 9  |  |  |  |  |  |
| 10 |  |  |  |  |  |
| **media** |  |  |  |  |  |
| **σ**     |  |  |  |  |  |

#### 30.0 s (teorico 30 000 ms)

| # | t_on (ms) | t_off (ms) | T_misurato (ms) | errore (ms) | pass (< 2 ms)? |
|---|---|---|---|---|---|
| 1  |  |  |  |  |  |
| 2  |  |  |  |  |  |
| 3  |  |  |  |  |  |
| 4  |  |  |  |  |  |
| 5  |  |  |  |  |  |
| 6  |  |  |  |  |  |
| 7  |  |  |  |  |  |
| 8  |  |  |  |  |  |
| 9  |  |  |  |  |  |
| 10 |  |  |  |  |  |
| **media** |  |  |  |  |  |
| **σ**     |  |  |  |  |  |

#### 60.0 s (teorico 60 000 ms)

| # | t_on (ms) | t_off (ms) | T_misurato (ms) | errore (ms) | pass (< 2 ms)? |
|---|---|---|---|---|---|
| 1  |  |  |  |  |  |
| 2  |  |  |  |  |  |
| 3  |  |  |  |  |  |
| 4  |  |  |  |  |  |
| 5  |  |  |  |  |  |
| 6  |  |  |  |  |  |
| 7  |  |  |  |  |  |
| 8  |  |  |  |  |  |
| 9  |  |  |  |  |  |
| 10 |  |  |  |  |  |
| **media** |  |  |  |  |  |
| **σ**     |  |  |  |  |  |

### Pass criterion V1

- [ ] Tutti i 30 campioni (10 × 3 durate) con `|errore| < 2 ms` → REQ-1.3 soddisfatto.
- [ ] Errore massimo osservato su qualsiasi campione < 1 ms → REQ-1.2 soddisfatto (entro tick ISR).
- [ ] Nessun campione anomalo (outlier > 5 ms) sintomo di interferenza I/O (SoftwareSerial 7-seg o I²C LCD).

---

## V2 — Jitter metronomo (REQ-1.5) — CHECKLIST OPERATORE

**Target REQ-1.5:** σ del periodo tick metronomo < 1 ms su 30 s di registrazione.

### Setup

- [ ] Microfono condensatore (smartphone o USB) posizionato a ~10 cm dal buzzer (pin 49).
- [ ] Software: Audacity (freeware), registrazione mono 44.1 kHz 16-bit.
- [ ] Ambiente silenzioso (no AC, no computer fan vicino).

### Procedura

1. COUNTDOWN, SECONDS (irrilevante), `timeSet = 300` (30.0 s), canale A.
2. Avvia registrazione Audacity, premi main button.
3. Lascia scadere il countdown fino al tono di stop.
4. Stoppa registrazione.
5. In Audacity: zoom sul waveform, trova i picchi dei 30 tick metronomo (TONE_DOWN 80 ms, onda a 300 Hz riconoscibile per pattern ripetuto).
6. Usa "Analyze → Contrast" o marcatori manuali per annotare il timestamp del fronte di salita di ogni tick.
7. Calcola gli intervalli tra tick consecutivi (29 intervalli).
8. Calcola media, deviazione standard, min, max.

### Risultati (compilare da operatore umano)

| Tick # | Timestamp (s, HH:MM:SS.mmm) | Δt vs precedente (ms) |
|---|---|---|
| 1  |  | — |
| 2  |  |  |
| 3  |  |  |
| ... | ... | ... |
| 30 |  |  |

**Statistiche (su 29 intervalli):**

- Media intervallo: ______ ms (atteso: 1000 ms)
- σ (deviazione standard): ______ ms
- Min: ______ ms
- Max: ______ ms
- Bias (media − 1000): ______ ms

- [ ] Allegato: file `.aup3` o `.wav` di registrazione in percorso: `docs/refactoring-timing/verify/v2_metronome_30s.wav` **oppure** screenshot di Audacity con marcatori visibili in `v2_metronome_audacity.png`.

### Pass criterion V2

- [ ] σ < 1 ms su 29 intervalli → REQ-1.5 soddisfatto.
- [ ] Bias |media − 1000| < 0.5 ms (drift di clock Timer1 vs. standard assoluto — accettabile per uso fotografico).

---

## V3 — Test-strip step beep (REQ-1.4) — CHECKLIST OPERATORE

**Target REQ-1.4:** latenza beep step < 1 ms rispetto al tick teorico.

### Setup

- [ ] Come V2 (microfono + Audacity).
- [ ] Opzionale: logic analyzer sul pin 49 per misura indipendente di latenza trigger vs. fronte sonoro.

### Procedura

1. TEST_STRIP, CONTINOUS, SECONDS, precisione 2 s (encoder precisione in posizione 1).
2. Canale A, gate attivato.
3. Avvia registrazione Audacity, premi main button.
4. Attendi almeno 5 step (t = 10, 12, 14, 16, 18 s).
5. Stoppa registrazione.
6. Identifica i 5 step beep (TONE_UP 150 ms, onda a 600 Hz riconoscibile).
7. Per ciascun beep calcola `latenza = timestamp_beep - t_teorico` (t_teorico = 10, 12, 14, 16, 18 s dal main button).

### Risultati (compilare da operatore umano)

| Step # | t_teorico (s) | timestamp beep (s) | latenza (ms) | pass (< 1 ms)? |
|---|---|---|---|---|
| 1 | 10 |  |  |  |
| 2 | 12 |  |  |  |
| 3 | 14 |  |  |  |
| 4 | 16 |  |  |  |
| 5 | 18 |  |  |  |

- [ ] Allegato: `docs/refactoring-timing/verify/v3_teststrip_beeps.wav`.

### Pass criterion V3

- [ ] Tutti i 5 beep con `|latenza| < 1 ms` → REQ-1.4 soddisfatto.
- [ ] Nessuna deriva cumulativa tra step 1 e step 5 > 2 ms (verifica coerenza `g_elapsedMs` driven da ISR).

---

## V4 — Consistenza funzionale (REQ-3.*) — CHECKLIST OPERATORE

**Target REQ-3.1..3.6:** nessuna differenza osservabile nel comportamento utente vs. baseline `Libraries_Refactoring`.

Riferimento: `DOCS.md` / `DOCS-IT.md` (contratto utente) + `CLAUDE.md §11..§16` (dettagli tecnici di comportamento).

### Boot / splash / PRONTO

- [ ] LCD mostra splash `" - R080 Timer - "` + `"    - MEGA -    "` per ~3 s.
- [ ] Backlight LCD fisso **rosso**.
- [ ] Serial monitor (9600 baud) stampa `=== R080 TIMER PRONTO ===`.
- [ ] Dopo splash, display 7-seg mostra `0000` con DP a cifra 1 (o il valore EEPROM ripristinato se pre-configurato).
- [ ] Nessun carattere sporco sull'LCD.

### FOCUS (CLAUDE.md §11.1)

- [ ] Encoder mode sinistro in posizione FOCUS → riga 0 LCD = `"FOCUS           "`.
- [ ] Main button → relè ON (se canale attivato), 7-seg conta in avanti da 0.0 s a passo 0.1 s.
- [ ] Keypad digit (0..9) → **ignorati** (timer continua indisturbato).
- [ ] `*` → brightness -1 (luminosità 7-seg visibilmente diminuisce; min = 1).
- [ ] `#` → brightness +1 (luminosità aumenta; max = 10).
- [ ] Main button → stop, relè OFF, beep sequenza stop.
- [ ] Encoder precision → nessun effetto in FOCUS (no tipo precisione).

### COUNTDOWN (CLAUDE.md §11.2)

- [ ] `timeSet` via keypad (es. `3 0 0` → 30.0 s su 7-seg).
- [ ] Main button → relè ON, 7-seg decrementa a passo 0.1 s.
- [ ] Metronomo tick ogni 1 s (TONE_DOWN 300 Hz 80 ms).
- [ ] A `timeSet = 0`: relè OFF, sequenza stop beep, `timeSet` ripristinato al valore originale (**repeat-last-exposure**, REQ-3.5).
- [ ] Ri-press main button → countdown ricomincia dal valore ripristinato, NON da 0.
- [ ] Encoder precision → ciclo SECONDS ⇄ FSTOP (default SECONDS).

### TEST_STRIP (CLAUDE.md §11.3)

- [ ] Encoder mode → TEST_STRIP. LCD riga 0: `"TEST STRIP [S]CO"` (SECONDS default, CONTINOUS default).
- [ ] Modality toggle via pulsante encoder mode (sinistro) → `CO` ⇄ `PP` (STEP_BY_STEP).
- [ ] Precision toggle via pulsante encoder precision (destro) → `[S]` ⇄ `[F]`.
- [ ] SECONDS 2s: beep step a t = 10, 12, 14, 16 s con warning (TONE_UP 50 ms) 3 decis prima.
- [ ] FSTOP 2 (prec 1/2): beep step approx 10, 14, 20, 28, 40 decis (arrotondamento `round(t * sqrt(2))`).
- [ ] STEP_BY_STEP: relè OFF a ogni step, ri-premere main per proseguire, 7-seg mostra nuovo target.
- [ ] CONTINOUS: relè resta ON, beep ma conteggio continua.

### F-Stop navigation (CLAUDE.md §12)

Pre-condizione: COUNTDOWN o TEST_STRIP, precision FSTOP, tempo base `timeSet > 0`.

- [ ] Offset iniziale `" 00"` (riga 1 col 5..7) con tempo base mostrato su 7-seg.
- [ ] `*` keypad (non-hold) → offset `"-01"`, tempo aggiornato a `baseTime * 2^(-1/prec)`.
- [ ] `#` keypad (non-hold) → offset `"+01"` (o `" 00"` / `"+01"` a seconda dello stato), tempo aggiornato.
- [ ] Offset a `"+02"` mostra tempo `baseTime * 2^(2/prec)` clamped 1..9999.
- [ ] Offset visualizzato correttamente: `"+02"` / `"-01"` / `" 00"` (4 car: segno + 2 cifre + padding implicito, layout 3 colonne 5..7).

### Channel switch A/B/C + reset D + per-channel relay gate (CLAUDE.md §13)

- [ ] Keypad `A` → `Ch:A` su riga 1 col 9..12, `timeSet` caricato da `channelTime[0]`.
- [ ] Keypad `B` → `Ch:B`, `timeSet` caricato da `channelTime[1]`.
- [ ] Keypad `C` → `Ch:C`, `timeSet` caricato da `channelTime[2]`.
- [ ] Ogni canale preserva il tempo tra switch: set `10.0` su A, switch B set `20.0`, switch C set `30.0`, rientro A → mostra `10.0` su 7-seg.
- [ ] Keypad `D` (release) → `channelTime[current] = 0`, reset F-stop offset a 0, `timeSet = 0`.
- [ ] Keypad HOLD `#` → toggle gate canale corrente, icona ⚡ (col 15) appare/sparisce.
- [ ] Main button + gate OFF → timer parte ma relè fisico NON si accende (`relayStart` gated in `base.h`).

### Keypad HOLD (CLAUDE.md §13)

- [ ] HOLD `*` → toggle mute. Se unmute: beep conferma TONE_UP 200 ms. Icona ♪ col 14 appare/sparisce.
- [ ] HOLD `#` → già coperto sopra (gate canale).
- [ ] HOLD A/B/C/D → no-op (fall-through a IDLE).
- [ ] HOLD 0..9 → no-op (quirk documentato in CLAUDE.md §13).

### EEPROM persistenza (REQ-6.4 + §15)

Pre-condizione: firmware runnante, settings modificati (brightness=7, mute=ON, canale=B con gate attivato, `channelTime[0..2]=100/200/300`).

- [ ] Spegni alimentazione (remove power from Mega).
- [ ] Attendi 10 s per garantire che EEPROM write sia committato (asincrono).
- [ ] Riaccendi.
- [ ] Dopo splash: canale corrente = B (riga 1 `"Ch:B"`).
- [ ] Gate canale B = ON (⚡ visibile).
- [ ] Brightness 7-seg = valore preservato.
- [ ] Mute = ON (♪ visibile, nessun beep al prossimo tick modalità).
- [ ] Switching A → `timeSet = 100` (10.0 s), switching B → `200` (20.0 s), switching C → `300` (30.0 s). Tutti preservati.

### Ripristino F-Stop non-regressione

- [ ] Modo COUNTDOWN, precision FSTOP, set tempo via keypad `1 0 0` (10.0 s).
- [ ] Encoder precision in posizione `1/2` → nessun cambiamento al tempo (baseTime preservato su setBase).
- [ ] `*` → `"-01"`, tempo ≈ 7.1 s (`10 * 2^(-1/2)`). Display 7-seg mostra ~71 (decis).
- [ ] `#` → `" 00"`, tempo a 10.0 s.
- [ ] `#` → `"+01"`, tempo ≈ 14.1 s.
- [ ] Main button → countdown parte dal tempo corrente (non dal baseTime), fine countdown ripristina il tempo corrente (non il baseTime) come repeat-last-exposure.

### Non-regressione integrata

- [ ] Eseguire 3 cicli completi FOCUS → COUNTDOWN → TEST_STRIP senza anomalie (LCD, 7-seg, beep, relè).
- [ ] Nessun comportamento osservabile diverso da `Libraries_Refactoring` baseline installato come riferimento (re-flash branch precedente se confronto diretto serve).

### Pass criterion V4

- [ ] Tutte le voci sopra marcate [x] dall'operatore umano.
- [ ] Nessuna nota FAIL sulle voci critiche (boot, mode switch, countdown end, repeat-last-exposure, EEPROM).

---

## Firma operatore

- Operatore umano: __________________________ (nome)
- Data esecuzione: __________________________ (YYYY-MM-DD)
- Versione firmware testata: commit/hash __________________________ **oppure** "uncommitted Libraries_Refactoring + US-001..US-005 staged".
- Allegati raccolti: [ ] V1 tabelle · [ ] V2 audio `.wav` + screenshot · [ ] V3 audio `.wav`.

---

## Note residue / follow-up (identificati dall'agente)

1. **Branch policy:** il piano originale (Technical.md §13) prevedeva branch separato `Performance_Rewrite`. Le Fasi A–F sono invece state implementate direttamente su `Libraries_Refactoring` come changes non commitate. La decisione di creare/non creare `Performance_Rewrite` è del project owner. Impatto: se rollback necessario, è un semplice `git restore .` invece di `git checkout Libraries_Refactoring` + `git branch -D Performance_Rewrite`.
2. **Fase G opzionale 7-seg su Serial3:** Technical.md §14 cita una possibile mitigazione se V2 mostrasse jitter anomalo dovuto a `SoftwareSerial` del display 7-seg. Raccomandazione: lasciare in standby e attivare SOLO se V2 mostra σ > 1 ms in produzione.
3. **CLAUDE.md §5 include order:** il testo originale elenca ancora `arduino-timer.h` e non include `util/atomic.h` né `functions/timer_isr.h`. Raccomandazione: aggiornare il documento post-merge per riflettere il nuovo ordine load-bearing.
4. **`.pio/libdeps/uno/`:** CLAUDE.md cita questa directory come "stale". Nessun impatto sulle build `megaatmega2560`, ma volendo pulire è un semplice `rm -rf .pio/libdeps/uno`.
