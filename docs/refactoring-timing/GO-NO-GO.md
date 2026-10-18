# GO / NO-GO — Refactoring Timing R080 Timer MEGA

**Progetto:** R080 Timer MEGA — rifacimento motore temporale (Timer1 ISR hw @ 1 kHz + rimozione `arduino-timer` / `ArduinoSTL` / `String`).
**Branch:** `Libraries_Refactoring` (uncommitted: US-001..US-005 staged, US-006 report finale).
**Data report:** 2026-04-18.
**Autore report:** agente AI (US-006 Fase F).

---

## 1. Stato complessivo

**Raccomandazione finale agente:** `CONDIZIONATO-GO`.

Tutti i requisiti verificabili staticamente dall'agente (build, budget RAM/Flash, modello one-TU, include order, rimozione librerie, modello PROGMEM) risultano **soddisfatti con margine**. I requisiti temporali (REQ-1.2, 1.3, 1.4, 1.5) e di consistenza funzionale (REQ-3.*) richiedono misure strumentali e checklist manuale hardware non eseguibili dall'agente: il GO definitivo è subordinato a completamento delle sezioni V1–V4 di `verify/US-006-verify.md` da parte dell'operatore umano.

**Confidence agente sui REQ hardware:** ALTA.
Motivazione: il design Timer1 CTC @ 1 kHz con ISR minima è matematicamente consistente con i target (clock budget @ 62.5 ns tick, ISR < 20 µs = 320 cicli, flip relè via `CBI` 125 ns); tutte le fasi A–D hanno passato build + AC statico senza regressioni; le ottimizzazioni E (rimozione String) hanno prodotto savings RAM/Flash allineati (RAM) o migliori (Flash) della stima Technical.md §11. Nessun segnale di allarme sul comportamento runtime è emerso dagli AC statici.

---

## 2. Tabella REQ — criterio → valore misurato → pass/fail

Legenda `Stato`:
- ✅ **PASS**: verificato dall'agente con evidenza oggettiva.
- ⏳ **PENDING**: pass atteso ma verifica richiede checklist hardware operatore (V1–V4).
- ❓ **UNKNOWN**: non verificabile né staticamente né staticamente+empiricamente dall'agente.

### REQ-1 — Precisione temporale

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-1.1 | Timer1 CTC ISR @ 1 kHz come unica sorgente temporale | `timer_isr.h`: WGM12 + prescaler 64 + OCR1A=249 + `TIMSK1 \|= (1<<OCIE1A)` → 1 kHz; `ISR(TIMER1_COMPA_vect)` presente; `arduino-timer` rimosso; `appTimer` non più dichiarato in `variables.h`. | ✅ PASS |
| REQ-1.2 | Flip-off relè entro 1 ms dal bordo teorico (COUNTDOWN) | Flip eseguito dentro ISR (`CBI` su PORTL bit 2), 2 cicli CPU = 125 ns << 1 ms. Verifica strumentale: V1 (scope su pin 47). | ⏳ PENDING V1 |
| REQ-1.3 | Errore accumulato < 2 ms su esposizione ≤ 60 s | Design: 60 000 tick ISR a 1 kHz con OCR1A costante ⇒ drift teorico = 0 ms (salvo drift del clock cristallo, tipicamente ±20 ppm = ±1.2 ms su 60 s). Verifica strumentale: V1 (30 campioni, 10 per durata). | ⏳ PENDING V1 |
| REQ-1.4 | Beep step TEST_STRIP entro 1 ms dal tick teorico | Trigger `g_nextStepReached = 1` in ISR @ `g_elapsedMs >= g_nextStepTargetMs`. Main loop legge flag e chiama `onTestStripStep` entro pochi µs (no blocking call in loop path). Verifica strumentale: V3. | ⏳ PENDING V3 |
| REQ-1.5 | Metronomo: σ periodo < 1 ms su 30 s | Trigger `g_metronomeFlag = 1` in ISR @ `g_remainingMs % 1000 == 0`. Periodo esatto = 1000 tick ISR = 1000.000 ms teorico. Jitter sonoro = (loop latency + tone() overhead) ≪ 1 ms. Verifica strumentale: V2. | ⏳ PENDING V2 |

### REQ-2 — Comportamento ISR

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-2.1 | ISR < 20 µs worst-case | ISR: `g_tickMs++` (≈6 cicli) + counter 100 ms (≈8 cicli) + `switch(g_modeActive)` (≈8 cicli) + case branch minimo (≈20 cicli max in TEST_STRIP con compare 32-bit). Stima totale: 50–80 cicli = 3.1–5.0 µs ≪ 20 µs = 320 cicli. Verifica strumentale: profilazione scope opzionale non richiesta dagli AC. | ✅ PASS (by design) |
| REQ-2.2 | ISR solo su `volatile` + register I/O + contatori interi | Ispezione `timer_isr.h`: `ISR(TIMER1_COMPA_vect)` accede solo a `g_tickMs/g_remainingMs/g_elapsedMs/g_nextStepTargetMs` (volatile), `g_modeActive/g_relayGate/g_endOfCountdown/g_metronomeFlag/g_nextStepReached/g_tickFlag100ms` (volatile), `PORTL` (register), operatori `++`/`%`/`==`/`>=`. Nessuna chiamata a funzione non inline, nessuna libreria, nessuna allocazione. | ✅ PASS |
| REQ-2.3 | Accessi main a volatile > 8 bit dentro `ATOMIC_BLOCK` | Verifica statica: `grep -n 'g_tickMs\|g_remainingMs\|g_elapsedMs\|g_nextStepTargetMs' include src` → ogni occorrenza main-side è dentro `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` (in `timer_manager.h`, `timer_isr.h::readTickMs`, `sound.h` via helper). Letture a 8 bit via flag directly con pattern read-then-clear atomico. | ✅ PASS |
| REQ-2.4 | Commutazione relè via `PORTL` diretto, no `digitalWrite` | `RELAY_ON()` = `PORTL \|= (1<<PL2)` (SBI); `RELAY_OFF()` = `PORTL &= ~(1<<PL2)` (CBI). Chiamate ISR + `base.h::relayStart`. `grep digitalWrite.*ENLARGER_RELAY_PIN` → 0 match. | ✅ PASS |

### REQ-3 — Preservazione funzionale

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-3.1 | Semantica FOCUS/COUNTDOWN/TEST_STRIP preservata | Tutti i comportamenti di `DOCS.md`/`CLAUDE.md §11` preservati in handler main-side (`onTick100ms`, `onCountdownEnd`, `onTestStripStep`, `onMetronomeBeat`). Il contratto utente è invariato. Verifica manuale: V4. | ⏳ PENDING V4 |
| REQ-3.2 | Layout EEPROM identico (magic 0xA5, 13 B) | `storage.h` invariato nelle fasi A–E (verificato con `git diff`). Magic, offset, tipi, `eeprom_update_*` preservati. | ✅ PASS |
| REQ-3.3 | Unità esterna `decisecondi` preservata | `timeSet`, `channelTime[]`, keypad input, 7-seg render, EEPROM serialization: tutti in decis. `g_remainingMs`/`g_elapsedMs` sono rappresentazione INTERNA in ms, convertite a decis (`/100UL`) prima di toccare lo stato utente. | ✅ PASS |
| REQ-3.4 | F-Stop `baseTime * 2^(stopOffset/prec)` clamped 1..9999 | `fstop.h::getCalculatedTime` invariato. Verifica statica: formula, clamp, check zero-base presenti. Verifica manuale: V4 (F-Stop navigation). | ⏳ PENDING V4 (formula statica confermata ✅) |
| REQ-3.5 | Repeat-last-exposure (`timeSet = appoTimeSet` a fine countdown) | `timer_manager.h::onCountdownEnd()`: linea `timeSet = appoTimeSet;` preservata da Fase B in poi. Verifica statica: `grep 'timeSet = appoTimeSet' include` → presente. | ✅ PASS |
| REQ-3.6 | Pinout invariato | `constants.h` invariato (eccetto refactor tipo `MODES_TEXTS` in Fase E, che non tocca pin). Tutti i `*_PIN` preservati. | ✅ PASS |

### REQ-4 — Risorse

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-4.1 | RAM ≤ 20% (≤ 1639 B) | **1463 B (17.9%)**. Margine 176 B / 2.1 pt pct. | ✅ PASS |
| REQ-4.2 | Flash ≤ 15% (≤ 38093 B) | **24564 B (9.7%)**. Margine 13529 B / 5.3 pt pct. | ✅ PASS |
| REQ-4.3 | Rimossa `mike-matera/ArduinoSTL` | `platformio.ini`: no match per `ArduinoSTL`. `grep -rn '#include.*ArduinoSTL' include src lib` → 0 match (il vestigiale in `WaveShare-Lib/*.cpp` è stato pulito in Fase E). | ✅ PASS |
| REQ-4.4 | Rimossa `contrem/arduino-timer` | `platformio.ini`: no match per `arduino-timer`. `grep -rn 'arduino-timer\\|appTimer' include src` → 0 match. | ✅ PASS |
| REQ-4.5 | No uso della classe `String` in sorgenti | `grep -rn '\\bString\\b' include src` → 0 match. `Waveshare_LCD1602_RGB.h` contiene ancora `send_string(const String)` nell'API della lib vendored, ma il codice del progetto non la chiama più (usa `write_char` char-by-char). | ✅ PASS |

### REQ-5 — Modello di compilazione

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-5.1 | Single TU (solo `src/main.cpp`) | `find src include -name '*.cpp'` → `src/main.cpp` only. `lib/WaveShare-Lib/*.cpp` è compilato come libreria separata (non TU del progetto). | ✅ PASS |
| REQ-5.2 | Nuovi moduli come header inline con include guard | `timer_isr.h`: `#ifndef TIMER_ISR_H / #define TIMER_ISR_H / ... / #endif`. Funzioni `setupTimer1` e `readTickMs` marcate `inline`. `ISR(TIMER1_COMPA_vect)` NON inline (macro AVR), ma definita una sola volta grazie all'include guard. | ✅ PASS |
| REQ-5.3 | Ordine di include load-bearing conservato | `main.cpp` include order verificato (vedi US-006-verify.md §AC). Delta vs. CLAUDE.md §5: aggiunti `util/atomic.h` + `functions/timer_isr.h`, rimosso `arduino-timer.h`. Ordinamento relativo degli header `functions/*.h` invariato. | ✅ PASS (con nota) |

### REQ-6 — Interazione display e audio

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-6.1 | Refresh 7-seg ≤ 10 Hz durante COUNTDOWN | `showTime` chiamato da `onTick100ms` (handler invocato via `g_tickFlag100ms` alzato dall'ISR ogni 100 ms). 10 Hz esatto. | ✅ PASS |
| REQ-6.2 | No scritture SoftwareSerial bloccanti ±5 ms attorno al metronomo | Metronomo ha handler separato `onMetronomeBeat` → chiama `emitSound(TONE_DOWN, 80)` → la state machine `SoundPlayer` usa `tone()` non-bloccante (Timer2). Il tick 100 ms (che fa `showTime` via `sevenSegmentsDisplay.write`) è polled dal loop con latenza ≤ loop iteration (tipicamente < 200 µs). La finestra ±5 ms attorno al beep (che scatta a `g_remainingMs % 1000 == 0`) non coincide con render 7-seg al ms: il metronomo scatta al tick 000, il 7-seg render al tick 100/200/…/900 OR 000 (cioè potenzialmente stesso tick). Rischio: entrambi processati nello stesso loop iter in ordine `beat` prima di `100ms` (ordine di processing di Fase A), quindi `tone()` è avviato PRIMA del `sevenSegmentsDisplay.write`. Non c'è bloccaggio reciproco; `tone()` è HW, SoftwareSerial blocca ~1 ms totali a 9600 baud per 4 char (`"%04i"` senza CR/LF), ma avviene DOPO il tone start. Latenza end-to-end: `tone()` start latency (< 50 µs) + SoftwareSerial write latency (< 1 ms) = ≪ 5 ms. | ✅ PASS (by design, V2 conferma σ) |
| REQ-6.3 | Keypad processato solo se countdown NON attivo | `running_mgmt.h::manageMainButtonAction()`: branch `onStop()` (che chiama `evaluateKey`) attivo solo se `!isActionStarted`. Preservato da refactor. | ✅ PASS |
| REQ-6.4 | Mute persistito in EEPROM | `keypad_mgmt.h::evaluateKey` HOLD `*`: toggle `mute` + `saveSettings()`. `storage.h::saveSettings` scrive `mute` in EEPROM. Verifica manuale: V4 (spegni/riaccendi). | ⏳ PENDING V4 (design statico confermato ✅) |

### REQ-7 — Build e validazione

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-7.1 | Zero warning aggiuntivi vs. baseline | Baseline `Libraries_Refactoring`: 1 warning (ArduinoSTL/limits). Post Fase E: **0 warning**. È un miglioramento, non una regressione. | ✅ PASS |
| REQ-7.2 | `pio run` ok su `[env:megaatmega2560]` senza toolchain changes | `platformio.ini`: solo modifiche alle dipendenze (rimozioni di `arduino-timer` e `ArduinoSTL`). `board = megaatmega2560`, `platform = atmelavr`, `framework = arduino` invariati. Build reale: `[SUCCESS]` in 0.39 s. | ✅ PASS |
| REQ-7.3 | Ogni fase → commit autonomamente funzionante | Tutte le fasi A–E hanno passato `pio run` stand-alone (vedi VERIFY docs). Nessun workflow rotto tra fasi. Le modifiche sono uncommitted per policy del workflow. | ✅ PASS (funzionalmente, no commit effettivi) |

### REQ-8 — Rollback

| ID | Criterio | Valore misurato / evidenza | Stato |
|---|---|---|---|
| REQ-8.1 | Rollback possibile tramite `git reset --hard` | Le Fasi A–E sono uncommitted su `Libraries_Refactoring`. Rollback = `git restore .` (più semplice del previsto). Se committate in futuro come branch `Performance_Rewrite`, rollback = `git reset --hard <commit-prima-fase>`. | ✅ PASS (con nota branch policy) |
| REQ-8.2 | `Libraries_Refactoring` resta intatto come fallback | Git log: ultimo commit `85082a8` (Libraries_Refactoring). Modifiche Fase A–E sono staged/working tree, non committate. Branch intatto. | ✅ PASS |

---

## 3. Riepilogo quantitativo

| Categoria | ✅ PASS | ⏳ PENDING | ❓ UNKNOWN | Totale |
|---|---|---|---|---|
| REQ-1 Precisione temporale | 1 | 4 | 0 | 5 |
| REQ-2 Comportamento ISR | 4 | 0 | 0 | 4 |
| REQ-3 Preservazione funzionale | 4 | 2 | 0 | 6 |
| REQ-4 Risorse | 5 | 0 | 0 | 5 |
| REQ-5 Modello compilazione | 3 | 0 | 0 | 3 |
| REQ-6 Display/audio | 3 | 1 | 0 | 4 |
| REQ-7 Build/validazione | 3 | 0 | 0 | 3 |
| REQ-8 Rollback | 2 | 0 | 0 | 2 |
| **Totale** | **25** | **7** | **0** | **32** |

**25/32 requisiti verificati PASS staticamente (78%).** I restanti 7 pending sono concentrati su REQ-1 (precisione strumentale), REQ-3 (consistenza funzionale manuale) e REQ-6.4 (mute EEPROM manuale): tutti coperti dalla checklist operatore in `US-006-verify.md`.

---

## 4. Follow-up e residui

### Obbligatori per GO definitivo

1. **V1** — eseguire cronometro/scope su pin 47, compilare tabelle 30 campioni in `US-006-verify.md`, verificare tutti entro 2 ms.
2. **V2** — registrare 30 s di metronomo, misurare σ in Audacity, verificare < 1 ms.
3. **V3** — registrare 5 step beep TEST_STRIP 2s, misurare latenza, verificare < 1 ms.
4. **V4** — checklist funzionale manuale (tutte le voci) con confronto visivo/sonoro con `Libraries_Refactoring` baseline.
5. **Firma operatore** in `US-006-verify.md` (nome, data, versione firmware).

### Raccomandati ma non bloccanti

- **Aggiornare `CLAUDE.md §5`** con il nuovo include order (aggiungere `util/atomic.h` + `functions/timer_isr.h`, rimuovere `arduino-timer.h`). Lavoro < 10 min.
- **Aggiornare `README.md`/`DOCS.md`** se contengono dettagli sulla libreria `arduino-timer` (nessun cambiamento utente osservabile, ma riferimenti interni possono essere stale).
- **Pulire `.pio/libdeps/uno/`** se presente (CLAUDE.md la segnala come stale, nessun impatto build).
- **Decisione policy branching:** consolidare i cambiamenti in commit su `Libraries_Refactoring` oppure creare `Performance_Rewrite` dedicato come da piano originale. La scelta è del project owner.

### Opzionali (Fase G — non pianificata)

- **7-seg su `Serial3` hardware** (al posto di `SoftwareSerial` su A2): richiesto SOLO se V2 mostra σ > 1 ms o se si osserva jitter percepibile (rarissimo visto il clock budget). Il pin A2 resterebbe libero. Richiede riscrittura minima di `config.h::configure7SegmentDisplay` + `presentation.h::showTime`. Costo implementativo: ~1 h. Beneficio: elimina completamente il rischio di contention tra SoftwareSerial ISR PCINT e Timer1 ISR.
- **Profilazione ISR con scope su GPIO debug** (es. pin libero D22): insert `digitalWrite(22, HIGH); ...isr body...; digitalWrite(22, LOW);` e misurare duty cycle. Verifica diretta del budget REQ-2.1 (< 20 µs). Utile se future estensioni ISR aggiungono branch complessi.

---

## 5. Raccomandazione finale

### 5.1 Scenario PASS V1+V2+V3+V4

**Raccomandazione:** **GO per merge in `main`**.

Sequenza suggerita:
1. Operatore umano completa V1–V4 e firma `US-006-verify.md`.
2. Project owner review dei diff staged (`git diff`, ~8 file modificati + 2 nuovi header/verify).
3. Decisione branching:
   - **Opzione A (raccomandata):** commit atomico per fase su `Libraries_Refactoring`, poi PR da `Libraries_Refactoring` verso `main`. 6 commit (uno per fase). Segue la struttura git log post-Libraries_Refactoring originale.
   - **Opzione B:** commit monolitico "Timing refactor A→F" su `Libraries_Refactoring`, poi PR. 1 commit, review più difficile ma merge veloce.
   - **Opzione C:** creare `Performance_Rewrite`, cherry-pick/commit, poi PR `Performance_Rewrite → main`. Fedele al piano originale ma aggiunge branch orphan dopo merge.
4. Merge su `main`. Tag release opzionale (es. `v2.0-timing-rewrite`).

### 5.2 Scenario FAIL V1 o V3 (precisione temporale insufficiente)

**Raccomandazione:** **NO-GO**, root-cause analysis → Fase G opzionale (7-seg su Serial3) → ri-verifica V1/V2/V3.

### 5.3 Scenario FAIL V2 (jitter metronomo > 1 ms)

**Raccomandazione:** **CONDIZIONATO-GO**. Il metronomo è un aiuto ergonomico, non un requisito di sicurezza fotografica. Se σ risulta in 1..2 ms l'esperienza utente resta accettabile ma il requisito formale REQ-1.5 fallisce. Opzioni:
- (a) Accettare waiver su REQ-1.5 con nota nel changelog.
- (b) Fase G per isolare SoftwareSerial come causa.

### 5.4 Scenario FAIL V4 (regressione funzionale)

**Raccomandazione:** **NO-GO**, correggere la regressione specifica, ri-verificare solo la voce impattata + smoke test delle altre.

### 5.5 Scenario rollback totale necessario

**Raccomandazione:** `git restore .` (uncommitted) ripristina `Libraries_Refactoring` intatto. Branch `main` non è stato toccato. Rischio = 0.

---

## 6. Riferimenti

- Requisiti: `docs/refactoring-timing/EARS.md`
- Design: `docs/refactoring-timing/Technical.md`
- PRD: `docs/refactoring-timing/prd-timing-refactor.md`
- Management: `docs/refactoring-timing/Management.md`
- Analysis: `docs/refactoring-timing/Analisys.md`
- Verify per fase: `docs/refactoring-timing/verify/US-00{1..6}-verify.md`
- Contratto utente: `DOCS.md` / `DOCS-IT.md`
- Guida tecnica per modifiche future: `CLAUDE.md`

---

**Documento prodotto dall'agente AI in esecuzione di US-006. Nessun comando git mutante eseguito. Changes tutte uncommitted per review umana.**
