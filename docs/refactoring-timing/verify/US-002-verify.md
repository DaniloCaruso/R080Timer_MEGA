# US-002 — Fase B — Commutazione relè dentro l'ISR — Checklist di verifica hardware

**Data:** 2026-04-18
**Branch:** `Libraries_Refactoring` (working tree sporco, non committato)
**Firmware:** post-refactor Fase B (ISR estesa: COUNTDOWN decrementa `g_remainingMs` al ms, flip PORTL.PL2 al bordo preciso)

---

## 1. Obiettivo della Fase B

Spostare il flip del relè dell'ingranditore (pin D47 = PL2 su ATmega2560) dentro la ISR `TIMER1_COMPA_vect`, in modo che la fine esposizione COUNTDOWN avvenga **al bordo preciso del ms** (latenza ≤ 1 µs dal clock interno Timer1), indipendentemente dal carico del main loop. Il main loop esegue solo cleanup non critico via `onCountdownEnd()`.

**REQ di riferimento:** REQ-1.3 (errore esposizione ≤ 2 ms su durate standard).

---

## 2. Pre-requisiti hardware e strumenti

- [ ] Arduino Mega 2560 flashato con il firmware corrente:
  ```bash
  pio run --target upload
  ```
- [ ] Monitor seriale aperto a 9600 baud (`pio device monitor`)
- [ ] LCD 16×2 Waveshare RGB cablato (SDA=20, SCL=21)
- [ ] 7-segmenti SparkFun serial su pin A2 (TX)
- [ ] Keypad 4×4 su pin 27/29/31/33 (rows) + 35/37/39/41 (cols)
- [ ] Encoder modalità (3/4/17) ed encoder precisione (18/19/16) cablati
- [ ] Main button su pin 2 con `INPUT_PULLUP`
- [ ] Buzzer passivo su pin 49
- [ ] **Relè ingranditore cablato su pin D47** (PL2); verifica opzionale con LED in parallelo per ispezione visiva rapida
- [ ] **Strumento di misura di uno dei due tipi seguenti:**
  - Oscilloscopio digitale o analyzer logic sonde su D47 (GND comune); trigger sul fronte di salita, cursori per delta-t; **preferito per misura sub-ms**
  - _Alternativa_: cronometro digitale al ms con fotodiodo puntato sul LED di monitoring; trigger visuale sul fronte di salita/discesa

---

## 3. Verifiche già eseguite dall'agente (compile-time)

- [x] `find src include -name '*.cpp'` produce solo `src/main.cpp` (modello one-TU preservato)
- [x] `pio run --target clean && pio run` termina con `[SUCCESS]` ed exit code 0
- [x] Unico warning presente: `ArduinoSTL/limits: #warning limits header is nowhere complete or accurate` (**preesistente nel baseline Fase A**, non regressione Fase B)
- [x] Risorse: RAM 24.7 % (2025 B / 8192 B, invariato vs Fase A), Flash 11.1 % (28152 B / 253952 B, +138 B vs Fase A — macro ISR + handler cleanup)
- [x] Grep verifica macro: `RELAY_ON`/`RELAY_OFF` definite in `timer_isr.h`, usate in `base.h::relayStart()` e ISR (`RELAY_OFF()` in case 2 COUNTDOWN quando `g_remainingMs==0` e `g_relayGate==1`)
- [x] Grep verifica: nessun `digitalWrite(ENLARGER_RELAY_PIN, ...)` residuo nei percorsi di esecuzione timer (escluso `initializePinMode` in `config.h` che setta solo il reset post-pinMode — conforme)

---

## 4. Verifica pin mapping PL2 ↔ D47

Riferimento: datasheet ATmega2560 (Atmel Doc8154/8272/7930), sezione "I/O Ports → Port L". La tabella di mapping porta → Arduino pin del Mega 2560 è:

| Port L bit | Arduino pin |
|---|---|
| PL0 | D49 |
| PL1 | D48 |
| **PL2** | **D47** |
| PL3 | D46 |
| PL4 | D45 |
| PL5 | D44 |
| PL6 | D43 |
| PL7 | D42 |

- [ ] Ispezione visiva: sulla scheda Mega, il filo che va al relè dell'ingranditore è effettivamente collegato al pin etichettato `47` sulla header digitale

Nota: il commento in `include/functions/timer_isr.h` documenta questa corrispondenza e la fonte.

---

## 5. Checklist hardware (operatore umano)

Marcare `PASS` / `FAIL` su ogni voce. Annotare anomalie e numeri misurati dove richiesto.

### V1 — Canale A attivato: flip relè ON all'avvio COUNTDOWN

1. Accendere la scheda, attendere il boot
2. Verificare che canale A sia selezionato (LCD riga 1 col 9 mostra `Ch:A`) e che il gate ⚡ sia **ON** (LCD riga 1 col 15 mostra il simbolo ⚡). Se non è ON, tenere premuto `#` per togglare
3. Selezionare COUNTDOWN via encoder sinistro
4. Inserire `100` (10.0 s) da keypad
5. Premere il main button

- [ ] Allo start, pin D47 passa da LOW (0 V) a HIGH (5 V) entro ≤ 2 ms dalla pressione del main button (tollerance per debounce OneButton 50 ms + latenza `manageStarted()`)
- [ ] LED relè (se presente) o scope mostrano ON continuo per tutta la durata del countdown
- [ ] 7-segmenti decrementa regolarmente da `10.0` verso `0.0` con granularità 0.1 / 100 ms

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V2 — Misura cronometrica esposizione 10.0 s (5 campioni)

Setup: scope/analyzer su D47. Cursore A su fronte di salita (start); cursore B su fronte di discesa (end). Misura delta-t.

Impostazioni: COUNTDOWN, canale A attivato, timeSet = 100 (10.0 s), SECONDS precisione.

Eseguire 5 misurazioni consecutive (attendere reset display tra una e la successiva):

| # | Timestamp start | Delta misurato (s) | Errore vs 10.000 s (ms) | Pass (≤ 2 ms) |
|---|---|---|---|---|
| 1 |  |  |  | ☐ |
| 2 |  |  |  | ☐ |
| 3 |  |  |  | ☐ |
| 4 |  |  |  | ☐ |
| 5 |  |  |  | ☐ |

**Errore max osservato:** ______ ms
**Tutti i 5 campioni ≤ 2 ms:** ☐ SÌ ☐ NO

### V3 — Misura cronometrica esposizione 30.0 s (5 campioni)

Impostazioni: timeSet = 300 (30.0 s), canale A attivato.

| # | Timestamp start | Delta misurato (s) | Errore vs 30.000 s (ms) | Pass (≤ 2 ms) |
|---|---|---|---|---|
| 1 |  |  |  | ☐ |
| 2 |  |  |  | ☐ |
| 3 |  |  |  | ☐ |
| 4 |  |  |  | ☐ |
| 5 |  |  |  | ☐ |

**Errore max osservato:** ______ ms
**Tutti i 5 campioni ≤ 2 ms:** ☐ SÌ ☐ NO

### V4 — Misura cronometrica esposizione 60.0 s (5 campioni)

Impostazioni: timeSet = 600 (60.0 s), canale A attivato.

| # | Timestamp start | Delta misurato (s) | Errore vs 60.000 s (ms) | Pass (≤ 2 ms) |
|---|---|---|---|---|
| 1 |  |  |  | ☐ |
| 2 |  |  |  | ☐ |
| 3 |  |  |  | ☐ |
| 4 |  |  |  | ☐ |
| 5 |  |  |  | ☐ |

**Errore max osservato:** ______ ms
**Tutti i 5 campioni ≤ 2 ms:** ☐ SÌ ☐ NO

**Conformità REQ-1.3 (max 2 ms su 15 campioni totali):** ☐ PASS ☐ FAIL

### V5 — Canale con ⚡ disattivato: timer gira ma relè rimane sempre OFF

1. COUNTDOWN, canale A selezionato
2. Tenere premuto `#` per togglare il gate relè. Verificare che il simbolo ⚡ (riga 1 col 15) **sparisca** dall'LCD
3. Inserire `100` (10.0 s) e premere main button

- [ ] 7-segmenti decrementa normalmente da `10.0` a `0.0` e poi ritorna al valore iniziale `10.0` (repeat-last-exposure)
- [ ] Metronomo (TONE_DOWN 80 ms) emesso regolarmente ogni secondo
- [ ] Sequenza di stop sonora emessa a fine countdown
- [ ] **Pin D47 rimane LOW per tutta la durata del countdown** (verifica scope o LED sempre spento)
- [ ] Nessun glitch / spike transitorio su D47 al momento di `g_remainingMs==0` (la macro `if (g_relayGate) RELAY_OFF()` nella ISR protegge dalla commutazione)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V6 — Repeat-last-exposure: a fine countdown il 7-seg torna al valore iniziale

1. COUNTDOWN, canale A attivato (⚡ ON), timeSet = 100 (10.0 s)
2. Premere main button, attendere fine naturale del countdown

- [ ] Durante il countdown il 7-seg decrementa da `10.0` verso `0.0`
- [ ] All'istante di fine esposizione (relè OFF), la sequenza di stop sonora (3 beep TONE_DOWN) viene riprodotta
- [ ] Il 7-seg mostra `0.0` per un istante, poi (dopo `stopAllTimers()` che è chiamata dal loop `onStop()` successivo) torna stabilmente a `10.0`
- [ ] Una successiva pressione del main button riavvia un nuovo countdown da `10.0` (non da 0)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V7 — Stop manuale durante countdown: relè OFF immediato

1. COUNTDOWN, canale A attivato (⚡ ON), timeSet = 300 (30.0 s)
2. Avviare con main button
3. Dopo 10 s circa, **premere nuovamente il main button** per arresto manuale

- [ ] `pin D47` va a LOW entro ≤ 50 ms dalla pressione (debounce OneButton + chiamata `stopAllTimers()` → `relayStart(false)` → `RELAY_OFF()`)
- [ ] Sequenza stop sonora emessa
- [ ] 7-seg mostra il valore corrente al momento dello stop (non `0.0`, non `30.0`)
- [ ] Il gate ⚡ canale resta immutato (stop manuale non tocca `channelsRelayActivated[]`)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V8 — Assenza di regressione dei comportamenti pre-Fase B

Riscontro checklist completa Fase A (US-001-verify.md):

- [ ] V1 US-001 (boot banner) ancora PASS
- [ ] V2 US-001 (FOCUS conta crescente) ancora PASS
- [ ] V4 US-001 (TEST STRIP CONTINOUS) ancora PASS — nessun impatto Fase B
- [ ] V5 US-001 (switch canali con memoria) ancora PASS
- [ ] V6 US-001 (encoder modo/precisione) ancora PASS
- [ ] V7 US-001 (keypad digit + hold `*`/`#`) ancora PASS

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

---

## 6. Esito complessivo

- [ ] V1–V8 tutte PASS
- [ ] Errore max ≤ 2 ms su 15 campioni (5 × 3 durate: 10 s, 30 s, 60 s)
- [ ] Nessuna regressione osservabile vs comportamento Fase A
- [ ] Working tree git lasciato sporco (agente NON ha eseguito comandi git mutanti)

**Firma operatore:** ____________________________ **Data:** ______________

---

## 7. Note tecniche Fase B

### Architettura ISR estesa

L'ISR `TIMER1_COMPA_vect` ora esegue, oltre all'housekeeping di Fase A (`g_tickMs++`, flag 100 ms), uno `switch (g_modeActive)`. Il `case 2` (COUNTDOWN) decrementa `g_remainingMs` ogni ms e, nel ms in cui `g_remainingMs` raggiunge zero, esegue `if (g_relayGate) RELAY_OFF();` seguito da `g_endOfCountdown = 1`.

Il flip `PORTL &= ~(1 << PL2)` compila a una singola istruzione `CBI` (2 cicli di clock, 125 ns) → latenza end-of-exposure **sotto il microsecondo** rispetto al clock interno Timer1.

### Performance ISR (worst case Fase B, tutti i rami attivi)

| Operazione | Cicli stimati |
|---|---|
| Prologo ISR (push) | ~30 |
| `g_tickMs++` (32-bit) | ~20 |
| `switch (g_modeActive)` | ~5 |
| Decremento `g_remainingMs--` | ~20 |
| Check `== 0` + flip PORT + flag | ~6 |
| `g_100Counter` increment + check | ~8 |
| Epilogo ISR (pop + reti) | ~35 |
| **Totale worst case** | **~124 cicli ≈ 7.8 µs** |

Budget 20 µs ampiamente rispettato (margine > 2.5×). Compliant con REQ-2.1.

### Sequenza end-of-countdown (ordering garantito)

1. ISR al ms `T_end`: `g_remainingMs = 1 → 0`, `RELAY_OFF()` (se gate), `g_endOfCountdown = 1`
2. (stesso ISR) `g_100Counter = 99 → 100`, reset, `g_tickFlag100ms = 1`
3. Loop: ATOMIC_BLOCK legge e clear entrambi i flag
4. Loop processa `flagEnd` (chiama `onCountdownEnd`): setta `g_modeActive = 0` atomicamente, pulisce stato main-side, `emitStopSound()`, `timeSet = appoTimeSet`, `showTime(0)`
5. Loop processa `flag100` (chiama `onTick100ms`): legge `g_modeActive = 0`, skippa i rami (no render doppio)
6. Ciclo successivo: `isMainBtnPressed = false` → `onStop()` → `stopAllTimers()` → `RELAY_OFF()` idempotente + `showTime(timerCurrentValue = 0)`

Il rele' NON viene riaperto / richiuso durante il cleanup. L'unico fronte di discesa è al punto 1 nella ISR.

### Coerenza stato legacy

`timerCurrentValue` (decis, main-side) è ora una **vista derivata** da `g_remainingMs`: in `onTick100ms` case COUNTDOWN, letto atomicamente, convertito (ms → decis), scritto in `timerCurrentValue` per mantenere il contratto con `stopAllTimers()` e `emitMetronome()`. Nessuna duplicazione di verità.
