# US-005 — Fase E — Rimozione String/ArduinoSTL + migrazione PROGMEM

## Branch / Commit

- Branch: `Libraries_Refactoring`
- Stato: modifiche non commitate (review manuale prima del merge).

## Build

```
Processing megaatmega2560 (platform: atmelavr; board: megaatmega2560; framework: arduino)
...
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
...
Linking .pio/build/megaatmega2560/firmware.elf
Checking size .pio/build/megaatmega2560/firmware.elf
RAM:   [==        ]  17.9% (used 1463 bytes from 8192 bytes)
Flash: [=         ]   9.7% (used 24564 bytes from 253952 bytes)
Building .pio/build/megaatmega2560/firmware.hex
========================= [SUCCESS] Took 0.91 seconds =========================
```

### Metriche confronto

| Misura | Baseline (Fase D) | Post Fase E | Δ |
|---|---|---|---|
| RAM   | 2025 B (24.7%) | 1463 B (17.9%) | **−562 B** |
| Flash | 28520 B (11.2%) | 24564 B (9.7%) | **−3956 B** |

Target AC:
- RAM ≤ 20% (target ~1490/8192) → **1463 B (17.9%)** ✓
- Flash ≤ 15% (target ~22400/253952) → **24564 B (9.7%)** ✓

### Warning

Nessun warning di compilazione. Il preesistente warning `ArduinoSTL limits header` è stato eliminato con la rimozione della libreria.

## Acceptance Criteria — verifica statica (agente)

- [x] `types.h`: `PrecisionValueString`, `ModalityTypeStruct`, `PrecisionTypeStruct` usano `const char*`; costruttori aggiornati.
- [x] `types.h`: `Operation::name` è `const char*`; 4 costruttori (default, FOCUS, COUNTDOWN, TEST_STRIP) accettano `const char*`.
- [x] `FSTOP_PRECISIONS` e `SECONDS_PRECISIONS` usano POD `{uint8_t value; const char* str;}`. Stringhe messe in PROGMEM per risparmio RAM aggiuntivo.
- [x] `constants.h`: `MODES_TEXTS` in PROGMEM con `const char MODES_TEXT_N[] PROGMEM` per ciascuna stringa e `const char* const MODES_TEXTS[] PROGMEM` come array di puntatori.
- [x] `constants.h`: `CHANNELS_STR` come array di `const char*` in RAM.
- [x] `presentation.h`: firme `writePosition(int,int,const char*)` e `writeRow(int,const char*)`. Rimossi wrapper `String(...)`. Nuova `showModeName(uint8_t)` usa `pgm_read_ptr` + `strcpy_P` in `keyBuffer`. `showPrecision` legge stringa PROGMEM via `strcpy_P`.
- [x] `operations.h`: `getOperationByName(String)` rimossa; aggiunta `getOperationById(uint8_t)`. Rimosse anche le funzioni dead `currentOpDiffsFromLast`/`isLastOpEmpty`/`isCurrentOpEmpty` + metodi `isPrecisionChanged`/`isPrecisionTypeChanged`/`isModalityTypeChanged` di `Operation` (nessun callsite).
- [x] `platformio.ini`: rimossa `mike-matera/ArduinoSTL@^1.3.3`.
- [x] `grep -rn '\bString\b' include src` → 0 match.
- [x] `pio run` → `[SUCCESS]` exit 0.
- [x] RAM 17.9% ≤ 20%, Flash 9.7% ≤ 15%.
- [x] Zero warning di compilazione.
- [x] Nessun comando git mutante eseguito dall'agente.

## Checklist hardware (operatore umano)

**Setup:** flashare il firmware risultato di `pio run --target upload`, connettere l'hardware completo (LCD, display 7-seg, keypad, encoder, main button, buzzer, relè), alimentare.

Ogni voce: **PASS** / **FAIL** + breve nota se FAIL.

### Boot / splash

- [ ] All'alimentazione, LCD mostra schermata di benvenuto `" - R080 Timer - "` (riga 0) e `"    - MEGA -    "` (riga 1) per 3 secondi.
- [ ] Backlight LCD è **rosso** (fisso, non altri colori).
- [ ] Dopo splash, LCD riga 0 mostra il nome del modo corrente (FOCUS al primo boot senza EEPROM valida; ultimo modo attivo se EEPROM valida).
- [ ] Display 7-seg mostra `0000` con punto decimale alla prima cifra (`.`).
- [ ] Serial monitor a 9600 baud stampa `=== R080 TIMER PRONTO ===`.
- [ ] **Nessun carattere sporco o corrotto** sul display LCD al boot.

### Nomi modalità (verifica lettura PROGMEM)

Ruotare l'encoder di modalità (a sinistra) tra le 3 posizioni. Per ciascun modo verificare che la riga 0 dell'LCD mostri il nome corretto letto da PROGMEM:

- [ ] **FOCUS** → riga 0 mostra `"FOCUS           "` (FOCUS + 11 spazi, 16 char totali).
- [ ] **COUNTDOWN** → riga 0 mostra `"COUNTDOWN  [S]  "` (o `[F]` se precisione F-stop; `[ ]` fino a che precisione non è settata tramite encoder).
- [ ] **TEST STRIP** → riga 0 mostra `"TEST STRIP [S]CO"` (o `[F]CO`/`[S]PP`/`[F]PP` a seconda di precisione+modalità).
- [ ] Nessun residuo di caratteri dal modo precedente quando si commuta (es. ruotando da TEST STRIP a FOCUS non restano '[' ']' o 'CO' sulla riga 0).

### Precisione FSTOP (verifica stringhe precisione in PROGMEM)

Modo COUNTDOWN o TEST_STRIP, premere il pulsante encoder precisione (destro) per passare a **FSTOP** (la lettera su riga 0 col 12 deve diventare `F`). Ruotare encoder precisione in senso orario dall'inizio:

- [ ] Posizione 0: riga 1 mostra `"Pr:1/1  "` (8 char).
- [ ] Posizione 1: riga 1 mostra `"Pr:1/2  "`.
- [ ] Posizione 2: riga 1 mostra `"Pr:1/3  "`.
- [ ] Posizione 3: riga 1 mostra `"Pr:1/4  "`.
- [ ] Posizione 4: riga 1 mostra `"Pr:1/6  "`.
- [ ] Posizione 5: riga 1 mostra `"Pr:1/8  "`.
- [ ] Posizione 6: riga 1 mostra `"Pr:1/12 "`.
- [ ] Posizione 7: riga 1 mostra `"Pr:1/16 "`.
- [ ] Posizione 8: riga 1 mostra `"Pr:1/24 "`.
- [ ] Posizione 9: riga 1 mostra `"Pr:1/32 "`.
- [ ] Posizione 10: riga 1 mostra `"Pr:1/64 "`.

Ripetere in senso antiorario (passa dalla posizione 10 alla 0 con wrap): stessi valori osservati, nessun frame corrotto.

### Precisione SECONDS

Premere il pulsante encoder precisione per tornare a **SECONDS** (`S` su riga 0 col 12). Ruotare encoder:

- [ ] Posizione 0: `"Pr: 1s  "`.
- [ ] Posizione 1: `"Pr: 2s  "`.
- [ ] Posizione 2: `"Pr: 3s  "`.
- [ ] Posizione 3: `"Pr: 4s  "`.
- [ ] Posizione 4: `"Pr: 5s  "`.
- [ ] Posizione 5: `"Pr: 6s  "`.
- [ ] Posizione 6: `"Pr: 7s  "`.
- [ ] Posizione 7: `"Pr: 8s  "`.
- [ ] Posizione 8: `"Pr: 10s "`.
- [ ] Posizione 9: `"Pr: 15s "`.
- [ ] Posizione 10: `"Pr: 20s "`.

### Canale (verifica CHANNELS_STR in RAM)

Premere in sequenza keypad `A`, `B`, `C`:

- [ ] Premendo `A`: riga 1 col 9..12 mostra `"Ch:A"`.
- [ ] Premendo `B`: riga 1 col 9..12 mostra `"Ch:B"`.
- [ ] Premendo `C`: riga 1 col 9..12 mostra `"Ch:C"`.
- [ ] Icona ⚡ (col 15) visibile quando canale ha relay attivo; spazio vuoto quando disattivato (hold `#`).

### Offset F-stop (non-regressione Fase D)

Modo COUNTDOWN o TEST STRIP, precisione FSTOP, tempo base diverso da 0:

- [ ] Premere `*` → offset F-stop passa a `"-01"` su riga 1 col 5..7; tempo aggiornato.
- [ ] Premere `#` fino a `"+02"` visibile; tempo aggiornato.
- [ ] Tornare a offset `" 00"` e verificare tempo base identico a prima.

### Countdown non-regressione (Fasi A/B/C/D)

- [ ] Impostare 30 decis tramite keypad, premere main button → countdown parte, display 7-seg decrementa, metronomo tick ogni secondo.
- [ ] Rele' si spegne esattamente al raggiungimento di 0 (verifica sonora/visiva).
- [ ] Sequenza di stop (5 beep) si sente al termine.
- [ ] Ri-premere main button → countdown ricomincia da 30 decis (repeat-last-exposure funzionante).

### Test strip step beep (non-regressione Fase D)

- [ ] TEST_STRIP, CONTINOUS, SECONDS 2s, main button → beep di step a t=10/12/14/16/18s, con warning beep 3 decis prima.
- [ ] LCD e 7-seg si aggiornano senza flicker/corruzione durante il conteggio.

### Focus (non-regressione generale)

- [ ] FOCUS, main button → relè si accende (se canale attivo), 7-seg conta in avanti da 0.
- [ ] `*` → brightness -1, `#` → brightness +1 (luminosità 7-seg cambia visibilmente).

### Sintesi

- [ ] Operatore umano ha eseguito tutti i check sopra e marcato PASS su ciascuno (o annotato FAIL con nota).
- [ ] Nessun regresso funzionale rispetto a Fase D installata come referenza (re-flash branch precedente se serve confronto diretto).
