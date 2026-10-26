# TECHNICAL — Testi dell'LCD in più lingue

- **Feature:** `ui-languages`
- **Data:** 2026-10-26
- **Fonte delle decisioni:** sessione di grilling del 2026-10-26 (Q1–Q12); `docs/ui-languages/decisions.md`; codice di `feature/arduino-mega-firmware` al commit `0998376` (v2.0.0): `lib/r080_core/{Flash.h,Hal.h,Settings.h,Settings.cpp,Screen.h,Screen.cpp,ServiceMenu.h,ServiceMenu.cpp,Controller.h,Controller.cpp,Counters.h}`, `src/main.cpp`, `platformio.ini`, `test/test_timer/test_main.cpp`, `test/fakes/`; `TECNICO.md`, `DOCS-IT.md`, `DOCS.md`, `README.md`, `README-IT.md`, `CHANGELOG.md`; su `main` `site/installazione.html`, `site/flussi.html`.

## 1. Panoramica

Oggi i testi dell'LCD sono letterali sparsi in due file. `Screen::compose()` scrive `RESET: WDT`, le righe delle Modalità (`kModeRows`, con l'Avanzamento `CO` già dentro), `PP`, `St:`, `1/` e `Ch:`. `ServiceMenu` ha le tabelle `kItems`, `kCounterTitles`, `kHealthTitles`, `kCalTitles` e una ventina di formati `FSTR(...)` in `compose()`. Il menu è in italiano, la schermata principale in un misto di inglese (nomi delle Modalità) e abbreviazioni italiane (`CO`, `PP`). Non esiste un'impostazione di lingua.

L'obiettivo è un solo modulo di dominio, `Texts`, con una tabella in flash: un identificatore per testo, una colonna per lingua e il limite di colonne di ciascuno. `Screen` e `ServiceMenu` chiedono il testo con `text(id, lingua)`. La lingua è un nuovo byte di `GlobalRecord`, quindi il blocco impostazioni passa alla versione 2. Il Menu di servizio ha una nuova voce `Lingua/Language`. Con l'italiano, che è la lingua di fabbrica, l'LCD resta identico alla v2.0.0 e i test esistenti non cambiano.

## 2. Vincoli e invarianti

- **Dominio senza Arduino** (CLAUDE.md): `Texts` sta in `lib/r080_core/` e usa solo `Flash.h` (`PROGMEM`, `fbyte`) per la tabella in flash.
- **RAM:** niente `String` né allocazione dinamica. I testi restano in `PROGMEM` e non si copiano in RAM: `text()` restituisce il puntatore in flash, che `putF()`, `fmemcpy` e `fsnprintf` già accettano.
- **Niente blocchi nel loop:** il cambio di lingua non aggiunge attese. `Screen::render()` continua a mandare solo le differenze, al massimo una riga per giro.
- **EEPROM:** il salvataggio resta `Settings::service()` con `writeBlockStep()`, un byte per giro e `eeprom_update_*` nel driver. Il blocco impostazioni (`0x000`) passa da 43 a 44 byte e resta sotto il blocco contatori (`0x040` = 64).
- **Layout dell'LCD** (requisito 7.5 della feature `firmware-rewrite`): le posizioni non cambiano. I testi tradotti stanno nelle stesse colonne: Avanzamento 14–15, `Ch:` 9–11, `St:` 0–2, voci dopo `>` alle colonne 1–15.
- **Test** (CLAUDE.md): solo le due seam. I nuovi test vanno in `test/test_timer/` (dominio intero con driver finti), senza finti di moduli interni. Il controllo della tabella usa l'API pubblica di `Texts`.
- **Log USB:** i formati di `EventLog::print` in `Controller.cpp` e `Screen.cpp` (`BOOT`, `START`, `END`, `CANCEL`, `CAL`, `ERROR settings=factory`, `LCD offline/online`) non passano da `Texts` (REQ-1.4).
- **Commit, PR e issue** in inglese, integrazione con rebase and merge.

## 3. Modifiche

### C-1 — `lib/r080_core/Texts.{h,cpp}`: tabella dei testi (NUOVO)

- **Soddisfa:** REQ-1.1, REQ-1.2, REQ-1.3, REQ-1.5, REQ-1.6, REQ-1.7, REQ-4.1, REQ-4.2, REQ-4.3
- **Stato attuale:** nessun modulo dei testi. I letterali sono in `Screen.cpp` (`kModeRows`, `FSTR(...)`) e in `ServiceMenu.cpp` (`kItems[ITEM_COUNT][16]`, `kCounterTitles[4][16]`, `kHealthTitles[3][16]`, `kCalTitles[3][17]`, formati in `compose()`).
- **Modifica:**
  - `Texts.h` (NUOVO):
    ```cpp
    enum Language : uint8_t { LANG_IT, LANG_EN, LANG_COUNT };
    enum TextId : uint8_t { T_WDT_RESET, T_ADV_CONT, /* … un id per riga della tabella sotto … */ T_CLICK_CONFIRM, TEXT_COUNT };
    const char* text(TextId id, uint8_t lang);  // puntatore in flash; lang >= LANG_COUNT vale LANG_IT
    const char* languageName(uint8_t lang);     // "Italiano", "English": in flash, scritto nella sua lingua
    uint8_t textWidth(TextId id);               // colonne disponibili sull'LCD per il testo (limite per REQ-4.2)
    ```
  - `Texts.cpp` (NUOVO): `static const char kTexts[TEXT_COUNT][LANG_COUNT][17] PROGMEM` (circa 1,7 KB di flash, 0 byte di RAM), `static const uint8_t kWidth[TEXT_COUNT] PROGMEM` e `static const char kLanguageNames[LANG_COUNT][9] PROGMEM`. Ogni riga della tabella tiene insieme l'id, i testi delle lingue e il limite, così chi traduce vede il vincolo accanto al testo.
  - Per i formati (`_FMT`) il limite vale per il modello: `T_LEVEL_FMT` ha limite 15 perché `%d` può diventare `-99`, tre caratteri al posto di due. Gli altri formati non si allungano oltre 16.
  - Restano fuori dalla tabella, perché identici in ogni lingua (REQ-1.3): le righe delle Modalità (`FOCUS`, `COUNTDOWN  [S]`, `TEST STRIP [S]`), la schermata di avvio ` - R080 Timer - ` con `v<versione>`, `1/`, `R080 v%s`, `ppm: %+ld`, `%lu us`, `%lu`, `%luh %02lum`, `M%+d c%u  P%+d c%u` e il blocco pieno del Test LCD.
- **Tabella dei testi (approvata il 2026-10-26, Q-1).** "Max" è il numero di colonne disponibili. Le lunghezze sono calcolate.

| Id | Dove | Max | Italiano | Lung. | English | Lung. |
|---|---|---|---|---|---|---|
| `T_WDT_RESET` | Screen, riga 1 per 3 s dopo un reset da watchdog | 16 | `RESET: WDT` | 10 | `RESET: WDT` | 10 |
| `T_ADV_CONT` | Screen, riga 1 col. 14-15, test strip continuo | 2 | `CO` | 2 | `CO` | 2 |
| `T_ADV_STEP` | Screen, riga 1 col. 14-15, test strip passo-passo | 2 | `PP` | 2 | `SS` | 2 |
| `T_CHANNEL` | Screen, riga 2 col. 9-11 | 3 | `Ch:` | 3 | `Ch:` | 3 |
| `T_STEP` | Screen, riga 2 col. 0-2 durante il test strip | 3 | `St:` | 3 | `St:` | 3 |
| `T_MENU_TITLE` | Menu, riga 1 dell'elenco | 16 | `MENU SERVIZIO` | 13 | `SERVICE MENU` | 12 |
| `T_INFO` | voce | 15 | `Info` | 4 | `Info` | 4 |
| `T_COUNTERS` | voce | 15 | `Contatori` | 9 | `Counters` | 8 |
| `T_HEALTH` | voce | 15 | `Salute` | 6 | `Health` | 6 |
| `T_TEST_RELAY` | voce | 15 | `Test rele` | 9 | `Relay test` | 10 |
| `T_TEST_BUZZER` | voce | 15 | `Test buzzer` | 11 | `Buzzer test` | 11 |
| `T_TEST_SEVEN_SEG` | voce | 15 | `Test 7 segmenti` | 15 | `7-segment test` | 14 |
| `T_TEST_LCD` | voce | 15 | `Test LCD` | 8 | `LCD test` | 8 |
| `T_TEST_KEYPAD` | voce | 15 | `Test tastierino` | 15 | `Keypad test` | 11 |
| `T_TEST_ENCODERS` | voce | 15 | `Test encoder` | 12 | `Encoder test` | 12 |
| `T_TEST_BUTTON` | voce | 15 | `Test pulsante` | 13 | `Button test` | 11 |
| `T_CALIBRATION` | voce | 15 | `Calibrazione` | 12 | `Calibration` | 11 |
| `T_LCD_EXPOSURE` | voce | 15 | `LCD esposizione` | 15 | `LCD exposure` | 12 |
| `T_FOCUS_LIMIT` | voce | 15 | `Tempo max Focus` | 15 | `Focus max time` | 14 |
| `T_LANGUAGE` | voce (NUOVA), uguale in tutte le lingue | 15 | `Lingua/Language` | 15 | `Lingua/Language` | 15 |
| `T_FACTORY` | voce | 15 | `Ripristino fabb` | 15 | `Factory reset` | 13 |
| `T_EXIT` | voce | 15 | `Esci` | 4 | `Exit` | 4 |
| `T_POWER_ONS` | Contatori, titolo | 16 | `Accensioni` | 10 | `Power-ons` | 9 |
| `T_WDT_RESETS` | Contatori, titolo | 16 | `Reset watchdog` | 14 | `Watchdog resets` | 15 |
| `T_EXPOSURES` | Contatori, titolo | 16 | `Esposizioni` | 11 | `Exposures` | 9 |
| `T_LAMP_HOURS` | Contatori, titolo | 16 | `Ore lampada` | 11 | `Lamp hours` | 10 |
| `T_LOOP_MAX` | Salute, titolo | 16 | `Loop max` | 8 | `Loop max` | 8 |
| `T_ISR_MAX` | Salute, titolo | 16 | `ISR max` | 7 | `ISR max` | 7 |
| `T_LOST_TICKS` | Salute, titolo | 16 | `Tick persi` | 10 | `Lost ticks` | 10 |
| `T_CAL_LOG` | Calibrazione, titolo | 16 | `Log calibrazione` | 16 | `Calibration log` | 15 |
| `T_CAL_PPM` | Calibrazione, titolo | 16 | `Correzione ppm` | 14 | `PPM correction` | 14 |
| `T_CAL_MEASURE` | Calibrazione, titolo | 16 | `Durata 600 s` | 12 | `Measured 600 s` | 14 |
| `T_CLICK_START` | Log calibrazione, riga 2 | 16 | `Clic: avvia` | 11 | `Click: start` | 12 |
| `T_LOG_ACTIVE` | Log calibrazione attivo, riga 2 | 16 | `Attivo, D: esci` | 15 | `Active, D: exit` | 15 |
| `T_DURATION_FMT` | Durata 600 s, riga 2 (formato) | 16 | `%ld,%02ld s` | 11 | `%ld.%02ld s` | 11 |
| `T_LEVEL_FMT` | LCD esposizione, riga 2 (formato) | 15 | `Livello 1-8: %d` | 15 | `Level 1-8: %d` | 13 |
| `T_MINUTES_FMT` | Tempo max Focus, riga 2 (formato) | 16 | `Minuti: %ld` | 11 | `Minutes: %ld` | 12 |
| `T_RELAY_ON` | Test rele, riga 2 | 16 | `Clic: acceso` | 12 | `Click: on` | 9 |
| `T_RELAY_OFF` | Test rele, riga 2 | 16 | `Clic: spento` | 12 | `Click: off` | 10 |
| `T_SOUND_FMT` | Test buzzer, riga 2 (formato) | 16 | `Suono %u: clic` | 14 | `Sound %u: click` | 15 |
| `T_SEG_ALL` | Test 7 segmenti, riga 2 | 16 | `Accesi: 888.8` | 13 | `All on: 888.8` | 13 |
| `T_KEY_FMT` | Test tastierino, riga 2 (formato) | 16 | `Tasto: %c` | 9 | `Key: %c` | 7 |
| `T_PRESS` | Test pulsante, riga 2 | 16 | `Premi` | 5 | `Press` | 5 |
| `T_PRESSED` | Test pulsante, riga 2 | 16 | `Premuto` | 7 | `Pressed` | 7 |
| `T_RELEASED` | Test pulsante, riga 2 | 16 | `Rilasciato` | 10 | `Released` | 8 |
| `T_CONFIRM_Q` | Ripristino, riga 1 | 16 | `Confermi?` | 9 | `Confirm?` | 8 |
| `T_YES_NO` | Ripristino, riga 2 | 16 | `clic=Si D=No` | 12 | `click=Yes D=No` | 14 |
| `T_CLICK_RESET` | Ripristino, riga 2 | 16 | `Clic: ripristina` | 16 | `Click: reset` | 12 |
| `T_CLICK_CONFIRM` | altre voci, riga 2 | 16 | `Clic: conferma` | 14 | `Click: confirm` | 14 |

- **Comportamento atteso:** ogni testo dell'LCD si trova in un solo posto, con le sue traduzioni e il suo limite. Con `LANG_IT` i testi sono quelli della v2.0.0 (REQ-1.7).
- **Verifica:** test nuovo in `test/test_timer/test_main.cpp`. Per ogni `TextId` e ogni lingua controlla che `text()` sia non vuoto, abbia `strlen` ≤ `textWidth()` e contenga solo caratteri 0x20–0x7E. Controlla anche che `languageName()` esista per ogni lingua (REQ-4.1–4.3). Più `pio run` per il Mega, che verifica che la tabella stia in flash.

### C-2 — `lib/r080_core/Settings.{h,cpp}`: lingua nelle impostazioni, versione 2

- **Soddisfa:** REQ-3.1, REQ-3.2, REQ-3.3, REQ-3.4, REQ-3.5
- **Stato attuale:** `GlobalRecord {uint8_t activeChannel, mute, lcdLevel, segLevel, lcdExposureLevel, focusLimitIndex; int16_t clockPpm;}`, 8 byte. `static_assert(sizeof(SettingsData) == 38)`. In `Settings.cpp`: `VERSION = 1`, `BLOCK_LEN = 43`, `kFactory = {{0, 0, 3, 3, 1, 1, 0}, …}`. `inRange()` rifiuta indici fuori tabella e `load()` usa `kFactory` se il blocco non è valido.
- **Modifica:**
  - `GlobalRecord`: nuovo campo `uint8_t language;` dopo `focusLimitIndex`, prima di `clockPpm`. Così `int16_t` resta allineato e su AVR non c'è padding; il blocco si confronta comunque byte per byte con `static_assert`.
  - `static_assert(sizeof(SettingsData) == 39, …)`.
  - `Settings.cpp`: `VERSION = 2`, `BLOCK_LEN` diventa 44 (calcolato com'è oggi), `kFactory = {{0, 0, 3, 3, 1, 1, LANG_IT, 0}, …}`. Il commento dei valori di fabbrica cita la lingua italiana.
  - Mappa EEPROM `0x000`: `'R'`, `'8'`, versione 2, lunghezza 44 | `GlobalRecord` (9 byte) | 3 × `ChannelRecord` (30 byte) | CRC8.
  - `load()`: dopo il controllo, se `ok && d.g.language >= LANG_COUNT` allora `d.g.language = LANG_IT` e `load()` restituisce `true` (REQ-3.5). `inRange()` non controlla la lingua, così un valore sconosciuto non annulla le altre impostazioni.
  - Un blocco con versione 1 non passa `b[2] == VERSION` e porta a `kFactory` (REQ-3.4), già oggi senza codice nuovo.
  - `factoryReset()` resta `d = kFactory` e porta quindi la lingua a `LANG_IT` (REQ-3.3).
- **Comportamento atteso:** la lingua sopravvive allo spegnimento. Una scheda con impostazioni v2.0.0 riparte dai valori di fabbrica, con la riga di log `ERROR settings=factory`.
- **Verifica:** test in `test_timer`:
  - lingua inglese scelta dal menu, riavvio simulato (`Timer` nuovo sulla stessa `eeprom`), testi ancora in inglese;
  - blocco v1 scritto a mano in `eeprom.bytes` (`'R','8',1,43`, CRC giusto): valori di fabbrica e `ERROR settings=factory` nel log;
  - blocco v2 con CRC giusto e `language = 7`: italiano, altre impostazioni conservate (per esempio il Canale attivo);
  - Ripristino di fabbrica dopo l'inglese: menu in italiano.

### C-3 — `lib/r080_core/Controller.h`: lingua per `Screen`

- **Soddisfa:** REQ-1.2, REQ-2.4
- **Stato attuale:** `Controller` espone a `Screen` lo stato da mostrare (`muted()`, `channelIndex()`, `channel()`, `lcdLevel()`, `stripStep()`, `service()`).
- **Modifica:** nuovo `uint8_t language() const { return g().language; }` nella sezione "Stato letto da Screen".
- **Comportamento atteso:** `Screen` legge la lingua a ogni `compose()`, quindi un cambio vale dal giro successivo.
- **Verifica:** coperta dai test di C-4 e C-5.

### C-4 — `lib/r080_core/Screen.cpp`: testi della schermata principale e del reset

- **Soddisfa:** REQ-1.2, REQ-1.3, REQ-1.4, REQ-1.7, REQ-2.4
- **Stato attuale:** in `compose()`:
  - `fmemcpy(want_[0], FSTR("RESET: WDT"), 10)`;
  - `kModeRows = {"FOCUS           ", "COUNTDOWN  [S]  ", "TEST STRIP [S]CO"}`;
  - `fmemcpy(&want_[0][14], FSTR("PP"), 2)` per il passo-passo;
  - `FSTR("St:")` e `FSTR("Ch:")`.
- **Modifica:**
  - `uint8_t lang = ctl_.language();` all'inizio di `compose()`. Una funzione locale `putText(char* p, TextId id, uint8_t lang)` copia il testo da flash fino al terminatore, come `putF` di `ServiceMenu.cpp`.
  - `RESET: WDT` diventa `putText(want_[0], T_WDT_RESET, lang)`.
  - `kModeRows` perde l'Avanzamento: `"TEST STRIP [S]  "`. In test strip si scrive `putText(&want_[0][14], c.advance ? T_ADV_STEP : T_ADV_CONT, lang)`.
  - `St:` diventa `putText(want_[1], T_STEP, lang)` e `Ch:` diventa `putText(&want_[1][9], T_CHANNEL, lang)`. Le colonne dei numeri e della lettera del Canale restano fisse.
  - I `log_.print(FSTR("LCD offline"))` e `FSTR("LCD online")` non cambiano (REQ-1.4).
- **Comportamento atteso:** in inglese la riga 1 del test strip passo-passo è `TEST STRIP [S]SS`. Il resto della schermata principale è identico nelle due lingue con la tabella proposta. In italiano tutto è come nella v2.0.0.
- **Verifica:** i test esistenti di `test_timer` passano senza modifiche (REQ-1.7). Test nuovo: dopo la scelta dell'inglese, `TEST STRIP [S]CO` / `TEST STRIP [S]SS` e `RESET: WDT` al riavvio da watchdog.

### C-5 — `lib/r080_core/ServiceMenu.{h,cpp}`: testi tradotti e voce `Lingua/Language`

- **Soddisfa:** REQ-1.2, REQ-1.4, REQ-1.7, REQ-2.1, REQ-2.2, REQ-2.3, REQ-2.4, REQ-2.5
- **Stato attuale:**
  - enum delle voci `INFO … FOCUS_LIMIT, FACTORY, EXIT, ITEM_COUNT` (15 voci);
  - `kItems[ITEM_COUNT][16]`, `kPages[ITEM_COUNT]`, `kCounterTitles`, `kHealthTitles`, `kCalTitles`;
  - `compose()` scrive titolo e riga 2 con `putF` e `fsnprintf(b, sizeof b, FSTR(...))`;
  - `click()` dentro una voce salva con `set_.changed(); set_.save(); sound_.play(Sound::Confirm); inside_ = false;`.
- **Modifica:**
  - Enum delle voci: `LANGUAGE` fra `FOCUS_LIMIT` e `FACTORY` (16 voci). `kPages` riceve un `1` in quella posizione.
  - Le tabelle di letterali diventano tabelle di `TextId`:
    - `static const uint8_t kItemText[ITEM_COUNT] = {T_INFO, …, T_LANGUAGE, T_FACTORY, T_EXIT}`;
    - `kCounterText[4]`, `kHealthText[3]`, `kCalText[3]`.

    Le tabelle `char[..][16]` spariscono.
  - `compose()`: `uint8_t lang = set_.d.g.language;`. Ogni `FSTR("…")` tradotto diventa `text(T_…, lang)`, passato a `putF` o usato come formato di `fsnprintf` (puntatore in flash come oggi). `MENU SERVIZIO` diventa `text(T_MENU_TITLE, lang)`. Restano `FSTR` i formati identici in ogni lingua (C-1).
  - Voce `LANGUAGE` in `compose()`: titolo `text(T_LANGUAGE, lang)`, riga 2 `languageName(lang)` (REQ-2.2).
  - Voce `LANGUAGE` in `click()`, dentro la voce: `g.language = (g.language + 1) % LANG_COUNT;`, poi `set_.changed(); set_.save(); sound_.play(Sound::Confirm);`. A differenza delle altre voci `inside_` resta vero, così la riga 2 mostra subito il nuovo nome (REQ-2.3, REQ-2.5). `D` torna all'elenco, già nella nuova lingua (REQ-2.4). Comportamento confermato (Q-2).
- **Comportamento atteso:** in inglese l'elenco mostra `SERVICE MENU` e `>Relay test` e così via, `Lingua/Language` resta uguale e i test hardware e la calibrazione mostrano i testi inglesi. Il log di calibrazione su USB (`CAL ms=…`) non cambia.
- **Verifica:** test nuovi in `test_timer`:
  - la voce `>Lingua/Language` precede `>Ripristino fabb`;
  - dentro la voce compaiono `Italiano` e poi, dopo un clic, `English`, con il beep di conferma e il blocco salvato;
  - con `D` l'elenco mostra `SERVICE MENU`;
  - un secondo clic torna a `Italiano` (ciclo).

  I test esistenti del menu, che contano le rotazioni dell'encoder fino a una voce, si aggiornano dove la voce nuova sposta `Ripristino fabb` ed `Esci`.

### C-6 — `platformio.ini`, `CHANGELOG.md`: versione 2.1.0

- **Soddisfa:** REQ-5.1
- **Stato attuale:** `-DFW_VERSION=\"2.0.0\"`; `CHANGELOG.md` ha `## [2.0.0] — in collaudo`.
- **Modifica:** `-DFW_VERSION=\"2.1.0\"`. Nuova sezione `## [2.1.0]` con "Aggiunto: testi dell'LCD in italiano e inglese, voce Lingua/Language nel Menu di servizio" e, sotto "Da fare per aggiornare", "le impostazioni della v2.0.0 non vengono lette: si riparte dai valori di fabbrica (lingua italiana)".
- **Comportamento atteso:** la schermata di avvio e la voce Info mostrano `v2.1.0`.
- **Verifica:** `pio run`; il test della voce Info usa la versione passata dal test, quindi non cambia.

### C-7 — `DOCS-IT.md`, `DOCS.md`, `README.md`, `README-IT.md`, `TECNICO.md`: documentazione del firmware

- **Soddisfa:** REQ-5.2, REQ-5.4
- **Stato attuale:**
  - `DOCS-IT.md` §10 dice "I testi sull'LCD sono in italiano" ed elenca 15 voci; i valori di fabbrica (§ memoria) non citano la lingua. `DOCS.md` ha gli stessi contenuti in inglese.
  - `TECNICO.md` §7 descrive il blocco impostazioni a 43 byte, versione 1 e `GlobalRecord` a 8 byte; §9 cita "15 voci"; la tabella dei moduli non ha `Texts`.
  - I README elencano le funzioni del Menu di servizio.
- **Modifica:**
  - Manuali: voce `Lingua/Language` nella tabella del §10; "I testi sull'LCD sono in italiano o in inglese, a scelta nella voce Lingua/Language"; lingua italiana tra i valori di fabbrica; nota di aggiornamento dalla v2.0.0. In `DOCS.md` i nomi delle voci si citano nella forma inglese con l'italiano tra parentesi dove serve.
  - `TECNICO.md`: §7 con 44 byte, versione 2, `GlobalRecord` a 9 byte con la lingua; §9 con 16 voci; tabella dei moduli con `Texts.{h,cpp}`.
  - README (`README.md`, `README-IT.md`): "lingua dell'LCD (italiano/inglese)" nell'elenco del Menu di servizio e restyling nello stile del README di `main`. In apertura il banner `docs/assets/r080-banner.svg` (arriva da `main` con il rebase) e i badge: CI `firmware.yml` del branch, sito, licenza CC BY-NC-SA 4.0, versione 2.1.0. Poi i link alle pagine del sito e le funzioni in tabelle compatte; pinout, compilazione e documenti restano.
- **Comportamento atteso:** manuali e codice descrivono lo stesso menu.
- **Verifica:** rilettura; `grep -n "in italiano" DOCS-IT.md` non trova più la frase vecchia; anteprima del README su GitHub (banner e badge visibili, link funzionanti).

### C-8 — `site/installazione.html`, `site/flussi.html` su `main`: sito

- **Soddisfa:** REQ-5.3
- **Stato attuale:** `installazione.html` ha una scheda per voce del Menu di servizio (`<span class="lcdlabel">&gt;Info</span>` …, etichette in italiano anche nella versione inglese). `flussi.html` descrive i percorsi nel menu con i testi italiani.
- **Modifica:** con una PR separata su `main`, dopo il merge del firmware:
  - nuova scheda `>Lingua/Language` prima di `>Ripristino fabb`;
  - nella versione inglese del sito le etichette dell'LCD in inglese (`>Factory reset`, `Confirm?`);
  - in `flussi.html` il percorso per cambiare lingua.
- **Comportamento atteso:** il sito descrive la v2.1.0 nelle due lingue.
- **Verifica:** workflow `pages` verde, pagine raggiungibili (HTTP 200), controllo a vista di IT ed EN.

## 4. Ordine di implementazione

1. **C-1**: `Texts` e test della tabella. Compila da solo, nessuno lo usa ancora.
2. **C-2**: lingua nelle impostazioni, versione 2, con i test di persistenza. Dipende da C-1 (`LANG_IT`, `LANG_COUNT`).
3. **C-3 + C-4**: `Controller::language()` e schermata principale. Dipende da C-1 e C-2.
4. **C-5**: Menu di servizio tradotto e voce `Lingua/Language`. Dipende da C-1 e C-2; i test della lingua dalla schermata principale usano anche C-4.
5. **C-6 + C-7**: versione e documentazione del firmware, nella stessa PR. C-7 richiede il banner su `main` e il rebase del branch su `main`.
6. **C-8**: sito, con una PR su `main` dopo il merge della PR del firmware.

Ogni passo lascia `pio run` e `pio test -e native` verdi.

## 5. Rischi tecnici

| Rischio | Impatto | Mitigazione |
|---|---|---|
| Un testo tradotto troncato da `put`/`putF` senza errore | Scritta monca sull'LCD | Limite per testo in `kWidth` e test REQ-4.2; per i formati il limite tiene conto dell'espansione peggiore |
| Spostamento delle voci nel menu | I test esistenti che contano le rotazioni falliscono | Aggiornare i conteggi in C-5. I testi italiani non cambiano, quindi i confronti delle righe restano |
| Puntatore in flash usato come stringa in RAM | Testi illeggibili solo sul Mega (sul PC `PROGMEM` è vuoto e il test passa) | `text()` si usa solo con `putF`, `fmemcpy` e `fsnprintf`; controllo a vista sulla scheda nel collaudo (#16) |
| Flash e RAM | Circa 1,7 KB di flash in più, 0 di RAM | Ampio margine sul Mega; `pio run` riporta l'occupazione |
| Banner assente sul branch del firmware | Immagine rotta nel README v2 | Ribasare `feature/arduino-mega-firmware` su `main` dopo la PR del README di `main`, prima di C-7 |
| Impostazioni perse all'aggiornamento da v2.0.0 | L'utente rifà le impostazioni | Nessuna scheda ha la v2.0.0 installata; nota nel CHANGELOG |

## 6. Matrice di tracciabilità

| REQ | Modifiche | Verifica |
|---|---|---|
| REQ-1.1 | C-1 | Test della tabella: due lingue |
| REQ-1.2 | C-1, C-3, C-4, C-5 | Test di schermata principale e menu in inglese |
| REQ-1.3 | C-1, C-4 | Test: righe delle Modalità e avvio identici in inglese |
| REQ-1.4 | C-4, C-5 | Test esistenti del log (`START`, `END`, `CAL`) invariati con l'inglese |
| REQ-1.5 | C-1 | Test della tabella: caratteri 0x20–0x7E |
| REQ-1.6 | C-1 | Revisione: Screen e ServiceMenu usano solo `TextId` e `LANG_COUNT` |
| REQ-1.7 | C-1, C-4, C-5 | Test esistenti di `test_timer` invariati |
| REQ-2.1 | C-5 | Test: `>Lingua/Language` prima di `>Ripristino fabb` |
| REQ-2.2 | C-5 | Test: `Italiano` / `English` nella voce |
| REQ-2.3 | C-5 | Test: clic e ciclo |
| REQ-2.4 | C-3, C-4, C-5 | Test: `SERVICE MENU` subito dopo `D` |
| REQ-2.5 | C-5 | Test: beep di conferma e blocco scritto in EEPROM |
| REQ-3.1 | C-2 | Test: riavvio simulato con l'inglese |
| REQ-3.2 | C-2 | Test: EEPROM vergine porta all'italiano |
| REQ-3.3 | C-2 | Test: Ripristino di fabbrica dopo l'inglese |
| REQ-3.4 | C-2 | Test: blocco versione 1 porta ai valori di fabbrica |
| REQ-3.5 | C-2 | Test: lingua 7 con CRC valido |
| REQ-4.1 | C-1 | Test della tabella |
| REQ-4.2 | C-1 | Test della tabella |
| REQ-4.3 | C-1 | Test della tabella |
| REQ-5.1 | C-6 | `pio run`, CHANGELOG |
| REQ-5.2 | C-7 | Rilettura dei documenti |
| REQ-5.4 | C-7 | Anteprima del README su GitHub |
| REQ-5.3 | C-8 | Workflow `pages`, pagine raggiungibili |

## 7. Questioni aperte

- **Q-1 — Revisione della tabella dei testi:** risolta il 2026-10-26, tabella di C-1 approvata.
- **Q-2 — Dove si preme per cambiare lingua:** risolta il 2026-10-26, comportamento di C-5.
- **Q-3 — Dove si aggiorna il README:** risolta il 2026-10-26, README e manuali in C-7, sito in C-8.
