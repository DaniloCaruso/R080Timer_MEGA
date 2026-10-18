# US-001 — Fase A — Motore temporale Timer1 ISR — Checklist di verifica hardware

**Data:** 2026-04-18
**Branch:** `Libraries_Refactoring` (working tree sporco, non committato)
**Firmware:** post-refactor Fase A (Timer1 CTC @ 1 kHz, ISR minima)

---

## Pre-requisiti

- [ ] Arduino Mega 2560 flashato con il firmware corrente (`pio run --target upload`)
- [ ] Monitor seriale aperto a 9600 baud (`pio device monitor`)
- [ ] LCD 16×2 Waveshare RGB connesso via I²C (SDA 20 / SCL 21)
- [ ] Display 7 segmenti SparkFun serial su pin A2 (TX)
- [ ] Keypad 4×4 su pin 27/29/31/33 (rows) + 35/37/39/41 (cols)
- [ ] Encoder modalità (3/4/17) ed encoder precisione (18/19/16) cablati
- [ ] Main button su pin 2 con `INPUT_PULLUP`
- [ ] Buzzer passivo su pin 49
- [ ] Relè ingranditore su pin 47 (ottimale: LED di monitoring in parallelo)

---

## Verifiche compilazione (già eseguite dall'agente)

- [x] `find src include -name '*.cpp'` produce solo `src/main.cpp`
- [x] `pio run` termina con `[SUCCESS]` ed exit code 0
- [x] Nessun warning nuovo rispetto al baseline `Libraries_Refactoring`
    - L'unico warning presente (`ArduinoSTL/limits: #warning limits header is nowhere complete or accurate`) è preesistente nel baseline, emesso dalla libreria ArduinoSTL.
- [x] Risorse: RAM 24.7 % (2025 B / 8192 B), Flash 11.0 % (28014 B / 253952 B)

---

## Checklist hardware

L'operatore umano deve marcare `PASS` / `FAIL` su ogni voce e annotare anomalie.

### V1 — Boot banner

- [ ] All'accensione, dopo il messaggio di benvenuto su LCD e i 3 secondi di splash, sul monitor seriale appare esattamente la stringa:
  `=== R080 TIMER PRONTO ===`
- [ ] Nessun output spurio o garbage prima/dopo il banner
- [ ] LCD in modalità FOCUS (o ultima salvata da EEPROM), 7-segmenti spento con brightness corretta

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V2 — FOCUS conta crescente a 0.1/100 ms

1. Selezionare modalità FOCUS con l'encoder sinistro
2. Azzerare manualmente il tempo se necessario (canale D reset)
3. Premere il main button

- [ ] Il 7-segmenti parte da `0.0` e incrementa esattamente `0.1` ogni 100 ms
- [ ] A 10 s il display mostra `10.0`; a 20 s mostra `20.0` (verificare con cronometro — tolleranza ±100 ms accettabile in questa fase)
- [ ] Il relè (LED) rimane ON per tutta la durata (canale attivato)
- [ ] Una seconda pressione del main button arresta il conteggio, relè OFF, sequenza stop sonora emessa

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V3 — COUNTDOWN 10.0 s → 0 in ~10 s

1. Selezionare modalità COUNTDOWN
2. Inserire `100` da keypad (= 10.0 decis)
3. Premere il main button

- [ ] Il 7-segmenti parte da `10.0` e decrementa di `0.1` ogni 100 ms
- [ ] Metronomo (tono 300 Hz, 80 ms) emesso al secondo 9, 8, 7, …, 1, 0
- [ ] A 0 il relè va OFF, viene riprodotta la sequenza di stop (3 beep TONE_DOWN)
- [ ] Tempo totale da avvio a OFF misurato con cronometro: **10.0 s ±100 ms**
- [ ] Dopo lo stop il display torna a `10.0` (repeat last exposure via `appoTimeSet`)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V4 — TEST STRIP CONTINOUS conta crescente

1. Selezionare modalità TEST STRIP
2. Verificare modalità `CO` (CONTINOUS) in colonna 14 riga 0 del LCD
3. Impostare precisione SECONDS = 2 s
4. Azzerare tempo (reset canale D), quindi avviare con main button

- [ ] Il display parte da `0.0` e incrementa di `0.1` ogni 100 ms come FOCUS
- [ ] A `10.0` (target iniziale di default) beep di step `TONE_UP 150 ms`
- [ ] Beep di warning `TONE_UP 50 ms` a `9.7`, `9.8`, `9.9` (3 decis prima del target)
- [ ] In CONTINOUS il conteggio prosegue; prossimo step a `+2 s` (12.0) ecc.
- [ ] Arresto con main button funzionante

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V5 — Switch canali A/B/C con memoria tempo

1. In COUNTDOWN, canale A: inserire `50` (5.0 s) da keypad
2. Premere `B` → canale B: inserire `120` (12.0 s)
3. Premere `C` → canale C: inserire `200` (20.0 s)
4. Premere `A` — verificare `5.0`
5. Premere `B` — verificare `12.0`
6. Premere `C` — verificare `20.0`

- [ ] Ogni `switchToChannel` ripristina il tempo memorizzato
- [ ] Simbolo `Ch:A|B|C` aggiornato correttamente in riga 1 colonna 9 dell'LCD
- [ ] Dopo reboot (power-cycle), i tempi persistono (EEPROM via `saveSettings()`)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V6 — Encoder modo e precisione reattivi quando stopped

Con timer fermo (no timer attivo, `isActionStarted == false`):

- [ ] Ruotare encoder sinistro: LCD cicla FOCUS → COUNTDOWN → TEST STRIP → FOCUS…
- [ ] Ruotare encoder destro in COUNTDOWN o TEST STRIP: la label di precisione riga 1 cols 0..7 cambia in tempo reale
- [ ] Pressione encoder destro in COUNTDOWN/TEST STRIP commuta SECONDS ↔ FSTOP (label col 12)
- [ ] Pressione encoder sinistro in TEST STRIP commuta CONTINOUS ↔ STEP_BY_STEP (label col 14)
- [ ] Durante conteggio (timer attivo), la rotazione degli encoder è ignorata (comportamento pre-esistente, voluto)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

### V7 — Keypad digit entry e hold */# funzionanti

Con timer fermo e modalità COUNTDOWN:

- [ ] Digitazione `1`, `2`, `3`, `4` → display `0.1` → `1.2` → `12.3` → `123.4` (max 4 cifre)
- [ ] Un 5° tasto non cambia il display (clamp a 9999)
- [ ] Tasto `*` in SECONDS decrementa di `precisionValue * 10` decis (min 1)
- [ ] Tasto `#` in SECONDS incrementa di `precisionValue * 10` decis (max 9999)
- [ ] Hold `*` (> 1 s) → muto on/off, label `♪` in riga 1 col 14 aggiornata
- [ ] Hold `#` (> 1 s) → toggle relay-gate canale corrente, label `⚡` in col 15
- [ ] `A`/`B`/`C` → switch canale, `D` → reset canale corrente (time=0)

**Esito:** ☐ PASS ☐ FAIL — Note: ______________________________________

---

## Esito complessivo

- [ ] Tutte le voci V1–V7 sono PASS
- [ ] Nessuna regressione osservabile rispetto al comportamento del branch `Libraries_Refactoring` precedente al refactor
- [ ] Working tree git lasciato sporco (agente NON ha eseguito comandi git mutanti)

**Firma operatore:** ____________________________ **Data:** ______________

---

## Note tecniche

- **Metrica riferimento esposizione (REQ-1.3):** in Fase A la tolleranza ammessa è ≤ 100 ms. La misura sub-ms via scope del REQ-1.3 sarà validata solo dopo Fase B (ISR estesa con flip PORTL atomico in ISR).
- **ISR minima Fase A:** incrementa solo `g_tickMs` e alza `g_tickFlag100ms` ogni 100 tick. Tutta la logica mode-specifica (focus/countdown/teststrip) resta main-side dentro `onTick100ms()`. Il comportamento utente è preservato 1:1 dal refactor.
- **Jitter atteso Fase A:** simile al baseline polling arduino-timer (worst-case `loop()` che in questa build è dominato da `manageMainButtonAction()` + `tickSound()` + dispatcher flag, tipicamente < 2 ms). Le verifiche di jitter sub-ms rimandate a Fase B/D.
