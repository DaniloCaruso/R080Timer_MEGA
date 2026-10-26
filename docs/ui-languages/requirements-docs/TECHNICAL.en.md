# TECHNICAL — LCD texts in several languages

- **Feature:** `ui-languages`
- **Date:** 2026-10-26
- **Source of the decisions:** grilling session of 2026-10-26 (Q1–Q12); `docs/ui-languages/decisions.md`; code of `feature/arduino-mega-firmware` at commit `0998376` (v2.0.0): `lib/r080_core/{Flash.h,Hal.h,Settings.h,Settings.cpp,Screen.h,Screen.cpp,ServiceMenu.h,ServiceMenu.cpp,Controller.h,Controller.cpp,Counters.h}`, `src/main.cpp`, `platformio.ini`, `test/test_timer/test_main.cpp`, `test/fakes/`; `TECNICO.md`, `DOCS-IT.md`, `DOCS.md`, `README.md`, `README-IT.md`, `CHANGELOG.md`; on `main` `site/installazione.html`, `site/flussi.html`.

## 1. Overview

Today the LCD texts are literals scattered across two files. `Screen::compose()` writes `RESET: WDT`, the Mode rows (`kModeRows`, with the `CO` Advance built in), `PP`, `St:`, `1/` and `Ch:`. `ServiceMenu` has the tables `kItems`, `kCounterTitles`, `kHealthTitles`, `kCalTitles` and about twenty `FSTR(...)` formats in `compose()`. The menu is in Italian; the main screen mixes English (Mode names) and Italian abbreviations (`CO`, `PP`). There is no language setting.

The target is a single domain module, `Texts`, with a table in flash: one identifier per text, one column per language and the column limit of each text. `Screen` and `ServiceMenu` ask for a text with `text(id, language)`. The language is a new `GlobalRecord` byte, so the settings block moves to version 2. The Service menu gets a new `Lingua/Language` item. With Italian, the factory language, the LCD stays identical to v2.0.0 and the existing tests do not change.

## 2. Constraints and invariants

- **Domain without Arduino** (CLAUDE.md): `Texts` lives in `lib/r080_core/` and uses only `Flash.h` (`PROGMEM`, `fbyte`) for the table in flash.
- **RAM:** no `String` and no dynamic allocation. The texts stay in `PROGMEM` and are not copied to RAM: `text()` returns the flash pointer, which `putF()`, `fmemcpy` and `fsnprintf` already accept.
- **No blocking in the loop:** changing the language adds no waits. `Screen::render()` still sends only the differences, at most one row per pass.
- **EEPROM:** saving stays `Settings::service()` with `writeBlockStep()`, one byte per pass and `eeprom_update_*` in the driver. The settings block (`0x000`) grows from 43 to 44 bytes and stays below the counters block (`0x040` = 64).
- **LCD layout** (requirement 7.5 of the `firmware-rewrite` feature): positions do not change. Translated texts sit in the same columns: Advance 14–15, `Ch:` 9–11, `St:` 0–2, items after `>` in columns 1–15.
- **Tests** (CLAUDE.md): only the two seams. New tests go in `test/test_timer/` (whole domain with fake drivers), with no fakes of internal modules. The table check uses the public `Texts` API.
- **USB log:** the `EventLog::print` formats in `Controller.cpp` and `Screen.cpp` (`BOOT`, `START`, `END`, `CANCEL`, `CAL`, `ERROR settings=factory`, `LCD offline/online`) do not go through `Texts` (REQ-1.4).
- **Commits, PRs and issues** in English, integrated with rebase and merge.

## 3. Changes

### C-1 — `lib/r080_core/Texts.{h,cpp}`: text table (NEW)

- **Satisfies:** REQ-1.1, REQ-1.2, REQ-1.3, REQ-1.5, REQ-1.6, REQ-1.7, REQ-4.1, REQ-4.2, REQ-4.3
- **Current state:** no text module. Literals are in `Screen.cpp` (`kModeRows`, `FSTR(...)`) and in `ServiceMenu.cpp` (`kItems[ITEM_COUNT][16]`, `kCounterTitles[4][16]`, `kHealthTitles[3][16]`, `kCalTitles[3][17]`, formats in `compose()`).
- **Change:**
  - `Texts.h` (NEW):
    ```cpp
    enum Language : uint8_t { LANG_IT, LANG_EN, LANG_COUNT };
    enum TextId : uint8_t { T_WDT_RESET, T_ADV_CONT, /* … one id per row of the table below … */ T_CLICK_CONFIRM, TEXT_COUNT };
    const char* text(TextId id, uint8_t lang);  // flash pointer; lang >= LANG_COUNT means LANG_IT
    const char* languageName(uint8_t lang);     // "Italiano", "English": in flash, written in its own language
    uint8_t textWidth(TextId id);               // LCD columns available for the text (limit for REQ-4.2)
    ```
  - `Texts.cpp` (NEW): `static const char kTexts[TEXT_COUNT][LANG_COUNT][17] PROGMEM` (about 1.7 KB of flash, 0 bytes of RAM), `static const uint8_t kWidth[TEXT_COUNT] PROGMEM` and `static const char kLanguageNames[LANG_COUNT][9] PROGMEM`. Each table row keeps together the id, the texts of every language and the limit, so a translator sees the constraint next to the text.
  - For formats (`_FMT`) the limit applies to the template: `T_LEVEL_FMT` has limit 15 because `%d` can become `-99`, three characters instead of two. The other formats do not grow beyond 16.
  - Left out of the table because identical in every language (REQ-1.3): the Mode rows (`FOCUS`, `COUNTDOWN  [S]`, `TEST STRIP [S]`), the start screen ` - R080 Timer - ` with `v<version>`, `1/`, `R080 v%s`, `ppm: %+ld`, `%lu us`, `%lu`, `%luh %02lum`, `M%+d c%u  P%+d c%u` and the LCD test's full block.
- **Text table (approved on 2026-10-26, Q-1).** "Max" is the number of available columns. Lengths are computed.

| Id | Where | Max | Italiano | Len | English | Len |
|---|---|---|---|---|---|---|
| `T_WDT_RESET` | Screen, row 1 for 3 s after a watchdog reset | 16 | `RESET: WDT` | 10 | `RESET: WDT` | 10 |
| `T_ADV_CONT` | Screen, row 1 cols 14-15, continuous test strip | 2 | `CO` | 2 | `CO` | 2 |
| `T_ADV_STEP` | Screen, row 1 cols 14-15, step-by-step test strip | 2 | `PP` | 2 | `SS` | 2 |
| `T_CHANNEL` | Screen, row 2 cols 9-11 | 3 | `Ch:` | 3 | `Ch:` | 3 |
| `T_STEP` | Screen, row 2 cols 0-2 during a test strip | 3 | `St:` | 3 | `St:` | 3 |
| `T_MENU_TITLE` | Menu, row 1 of the item list | 16 | `MENU SERVIZIO` | 13 | `SERVICE MENU` | 12 |
| `T_INFO` | item | 15 | `Info` | 4 | `Info` | 4 |
| `T_COUNTERS` | item | 15 | `Contatori` | 9 | `Counters` | 8 |
| `T_HEALTH` | item | 15 | `Salute` | 6 | `Health` | 6 |
| `T_TEST_RELAY` | item | 15 | `Test rele` | 9 | `Relay test` | 10 |
| `T_TEST_BUZZER` | item | 15 | `Test buzzer` | 11 | `Buzzer test` | 11 |
| `T_TEST_SEVEN_SEG` | item | 15 | `Test 7 segmenti` | 15 | `7-segment test` | 14 |
| `T_TEST_LCD` | item | 15 | `Test LCD` | 8 | `LCD test` | 8 |
| `T_TEST_KEYPAD` | item | 15 | `Test tastierino` | 15 | `Keypad test` | 11 |
| `T_TEST_ENCODERS` | item | 15 | `Test encoder` | 12 | `Encoder test` | 12 |
| `T_TEST_BUTTON` | item | 15 | `Test pulsante` | 13 | `Button test` | 11 |
| `T_CALIBRATION` | item | 15 | `Calibrazione` | 12 | `Calibration` | 11 |
| `T_LCD_EXPOSURE` | item | 15 | `LCD esposizione` | 15 | `LCD exposure` | 12 |
| `T_FOCUS_LIMIT` | item | 15 | `Tempo max Focus` | 15 | `Focus max time` | 14 |
| `T_LANGUAGE` | item (NEW), the same in every language | 15 | `Lingua/Language` | 15 | `Lingua/Language` | 15 |
| `T_FACTORY` | item | 15 | `Ripristino fabb` | 15 | `Factory reset` | 13 |
| `T_EXIT` | item | 15 | `Esci` | 4 | `Exit` | 4 |
| `T_POWER_ONS` | Counters, title | 16 | `Accensioni` | 10 | `Power-ons` | 9 |
| `T_WDT_RESETS` | Counters, title | 16 | `Reset watchdog` | 14 | `Watchdog resets` | 15 |
| `T_EXPOSURES` | Counters, title | 16 | `Esposizioni` | 11 | `Exposures` | 9 |
| `T_LAMP_HOURS` | Counters, title | 16 | `Ore lampada` | 11 | `Lamp hours` | 10 |
| `T_LOOP_MAX` | Health, title | 16 | `Loop max` | 8 | `Loop max` | 8 |
| `T_ISR_MAX` | Health, title | 16 | `ISR max` | 7 | `ISR max` | 7 |
| `T_LOST_TICKS` | Health, title | 16 | `Tick persi` | 10 | `Lost ticks` | 10 |
| `T_CAL_LOG` | Calibration, title | 16 | `Log calibrazione` | 16 | `Calibration log` | 15 |
| `T_CAL_PPM` | Calibration, title | 16 | `Correzione ppm` | 14 | `PPM correction` | 14 |
| `T_CAL_MEASURE` | Calibration, title | 16 | `Durata 600 s` | 12 | `Measured 600 s` | 14 |
| `T_CLICK_START` | Calibration log, row 2 | 16 | `Clic: avvia` | 11 | `Click: start` | 12 |
| `T_LOG_ACTIVE` | Calibration log running, row 2 | 16 | `Attivo, D: esci` | 15 | `Active, D: exit` | 15 |
| `T_DURATION_FMT` | Measured 600 s, row 2 (format) | 16 | `%ld,%02ld s` | 11 | `%ld.%02ld s` | 11 |
| `T_LEVEL_FMT` | LCD exposure, row 2 (format) | 15 | `Livello 1-8: %d` | 15 | `Level 1-8: %d` | 13 |
| `T_MINUTES_FMT` | Focus max time, row 2 (format) | 16 | `Minuti: %ld` | 11 | `Minutes: %ld` | 12 |
| `T_RELAY_ON` | Relay test, row 2 | 16 | `Clic: acceso` | 12 | `Click: on` | 9 |
| `T_RELAY_OFF` | Relay test, row 2 | 16 | `Clic: spento` | 12 | `Click: off` | 10 |
| `T_SOUND_FMT` | Buzzer test, row 2 (format) | 16 | `Suono %u: clic` | 14 | `Sound %u: click` | 15 |
| `T_SEG_ALL` | 7-segment test, row 2 | 16 | `Accesi: 888.8` | 13 | `All on: 888.8` | 13 |
| `T_KEY_FMT` | Keypad test, row 2 (format) | 16 | `Tasto: %c` | 9 | `Key: %c` | 7 |
| `T_PRESS` | Button test, row 2 | 16 | `Premi` | 5 | `Press` | 5 |
| `T_PRESSED` | Button test, row 2 | 16 | `Premuto` | 7 | `Pressed` | 7 |
| `T_RELEASED` | Button test, row 2 | 16 | `Rilasciato` | 10 | `Released` | 8 |
| `T_CONFIRM_Q` | Factory reset, row 1 | 16 | `Confermi?` | 9 | `Confirm?` | 8 |
| `T_YES_NO` | Factory reset, row 2 | 16 | `clic=Si D=No` | 12 | `click=Yes D=No` | 14 |
| `T_CLICK_RESET` | Factory reset, row 2 | 16 | `Clic: ripristina` | 16 | `Click: reset` | 12 |
| `T_CLICK_CONFIRM` | other items, row 2 | 16 | `Clic: conferma` | 14 | `Click: confirm` | 14 |

- **Expected behaviour:** every LCD text lives in one place, with its translations and its limit. With `LANG_IT` the texts are those of v2.0.0 (REQ-1.7).
- **Verification:** new test in `test/test_timer/test_main.cpp`. For every `TextId` and every language it checks that `text()` is non-empty, has `strlen` ≤ `textWidth()` and contains only characters 0x20–0x7E. It also checks that `languageName()` exists for every language (REQ-4.1–4.3). Plus `pio run` for the Mega, which checks that the table fits in flash.

### C-2 — `lib/r080_core/Settings.{h,cpp}`: language in the settings, version 2

- **Satisfies:** REQ-3.1, REQ-3.2, REQ-3.3, REQ-3.4, REQ-3.5
- **Current state:** `GlobalRecord {uint8_t activeChannel, mute, lcdLevel, segLevel, lcdExposureLevel, focusLimitIndex; int16_t clockPpm;}`, 8 bytes. `static_assert(sizeof(SettingsData) == 38)`. In `Settings.cpp`: `VERSION = 1`, `BLOCK_LEN = 43`, `kFactory = {{0, 0, 3, 3, 1, 1, 0}, …}`. `inRange()` rejects out-of-table indexes and `load()` uses `kFactory` when the block is invalid.
- **Change:**
  - `GlobalRecord`: new field `uint8_t language;` after `focusLimitIndex`, before `clockPpm`. This keeps `int16_t` aligned, and on AVR there is no padding; the block is checked byte for byte by `static_assert` anyway.
  - `static_assert(sizeof(SettingsData) == 39, …)`.
  - `Settings.cpp`: `VERSION = 2`, `BLOCK_LEN` becomes 44 (computed as today), `kFactory = {{0, 0, 3, 3, 1, 1, LANG_IT, 0}, …}`. The factory-values comment mentions the Italian language.
  - EEPROM map `0x000`: `'R'`, `'8'`, version 2, length 44 | `GlobalRecord` (9 bytes) | 3 × `ChannelRecord` (30 bytes) | CRC8.
  - `load()`: after the check, if `ok && d.g.language >= LANG_COUNT` then `d.g.language = LANG_IT` and `load()` returns `true` (REQ-3.5). `inRange()` does not check the language, so an unknown value does not discard the other settings.
  - A block with version 1 fails `b[2] == VERSION` and leads to `kFactory` (REQ-3.4), already today with no new code.
  - `factoryReset()` stays `d = kFactory` and therefore sets the language to `LANG_IT` (REQ-3.3).
- **Expected behaviour:** the language survives power-off. A board with v2.0.0 settings restarts from factory values, with the log line `ERROR settings=factory`.
- **Verification:** tests in `test_timer`:
  - English chosen from the menu, simulated restart (new `Timer` on the same `eeprom`), texts still in English;
  - a version 1 block written by hand into `eeprom.bytes` (`'R','8',1,43`, correct CRC): factory values and `ERROR settings=factory` in the log;
  - a version 2 block with correct CRC and `language = 7`: Italian, other settings kept (for example the active Channel);
  - Factory reset after English: menu in Italian.

### C-3 — `lib/r080_core/Controller.h`: language for `Screen`

- **Satisfies:** REQ-1.2, REQ-2.4
- **Current state:** `Controller` exposes to `Screen` the state to show (`muted()`, `channelIndex()`, `channel()`, `lcdLevel()`, `stripStep()`, `service()`).
- **Change:** new `uint8_t language() const { return g().language; }` in the "State read by Screen" section.
- **Expected behaviour:** `Screen` reads the language at every `compose()`, so a change applies from the next pass.
- **Verification:** covered by the tests of C-4 and C-5.

### C-4 — `lib/r080_core/Screen.cpp`: main screen and reset texts

- **Satisfies:** REQ-1.2, REQ-1.3, REQ-1.4, REQ-1.7, REQ-2.4
- **Current state:** in `compose()`:
  - `fmemcpy(want_[0], FSTR("RESET: WDT"), 10)`;
  - `kModeRows = {"FOCUS           ", "COUNTDOWN  [S]  ", "TEST STRIP [S]CO"}`;
  - `fmemcpy(&want_[0][14], FSTR("PP"), 2)` for step-by-step;
  - `FSTR("St:")` and `FSTR("Ch:")`.
- **Change:**
  - `uint8_t lang = ctl_.language();` at the start of `compose()`. A local function `putText(char* p, TextId id, uint8_t lang)` copies the text from flash up to the terminator, like `putF` in `ServiceMenu.cpp`.
  - `RESET: WDT` becomes `putText(want_[0], T_WDT_RESET, lang)`.
  - `kModeRows` loses the Advance: `"TEST STRIP [S]  "`. In test strip the code writes `putText(&want_[0][14], c.advance ? T_ADV_STEP : T_ADV_CONT, lang)`.
  - `St:` becomes `putText(want_[1], T_STEP, lang)` and `Ch:` becomes `putText(&want_[1][9], T_CHANNEL, lang)`. The columns of the numbers and of the Channel letter stay fixed.
  - `log_.print(FSTR("LCD offline"))` and `FSTR("LCD online")` do not change (REQ-1.4).
- **Expected behaviour:** in English row 1 of a step-by-step test strip is `TEST STRIP [S]SS`. The rest of the main screen is identical in both languages with the proposed table. In Italian everything is as in v2.0.0.
- **Verification:** the existing `test_timer` tests pass unchanged (REQ-1.7). New test: after choosing English, `TEST STRIP [S]CO` / `TEST STRIP [S]SS`, and `RESET: WDT` on a watchdog restart.

### C-5 — `lib/r080_core/ServiceMenu.{h,cpp}`: translated texts and `Lingua/Language` item

- **Satisfies:** REQ-1.2, REQ-1.4, REQ-1.7, REQ-2.1, REQ-2.2, REQ-2.3, REQ-2.4, REQ-2.5
- **Current state:**
  - item enum `INFO … FOCUS_LIMIT, FACTORY, EXIT, ITEM_COUNT` (15 items);
  - `kItems[ITEM_COUNT][16]`, `kPages[ITEM_COUNT]`, `kCounterTitles`, `kHealthTitles`, `kCalTitles`;
  - `compose()` writes title and row 2 with `putF` and `fsnprintf(b, sizeof b, FSTR(...))`;
  - `click()` inside an item saves with `set_.changed(); set_.save(); sound_.play(Sound::Confirm); inside_ = false;`.
- **Change:**
  - Item enum: `LANGUAGE` between `FOCUS_LIMIT` and `FACTORY` (16 items). `kPages` gets a `1` at that position.
  - The literal tables become `TextId` tables:
    - `static const uint8_t kItemText[ITEM_COUNT] = {T_INFO, …, T_LANGUAGE, T_FACTORY, T_EXIT}`;
    - `kCounterText[4]`, `kHealthText[3]`, `kCalText[3]`.

    The `char[..][16]` tables go away.
  - `compose()`: `uint8_t lang = set_.d.g.language;`. Every translated `FSTR("…")` becomes `text(T_…, lang)`, passed to `putF` or used as the `fsnprintf` format (a flash pointer, as today). `MENU SERVIZIO` becomes `text(T_MENU_TITLE, lang)`. Formats identical in every language stay `FSTR` (C-1).
  - `LANGUAGE` item in `compose()`: title `text(T_LANGUAGE, lang)`, row 2 `languageName(lang)` (REQ-2.2).
  - `LANGUAGE` item in `click()`, inside the item: `g.language = (g.language + 1) % LANG_COUNT;`, then `set_.changed(); set_.save(); sound_.play(Sound::Confirm);`. Unlike the other items, `inside_` stays true, so row 2 shows the new name at once (REQ-2.3, REQ-2.5). `D` returns to the list, already in the new language (REQ-2.4). Behaviour confirmed (Q-2).
- **Expected behaviour:** in English the list shows `SERVICE MENU`, `>Relay test` and so on, `Lingua/Language` stays the same, and the hardware tests and calibration show the English texts. The USB calibration log (`CAL ms=…`) does not change.
- **Verification:** new tests in `test_timer`:
  - the `>Lingua/Language` item comes before `>Ripristino fabb`;
  - inside the item `Italiano` appears, then `English` after a click, with the confirm beep and the block saved;
  - after `D` the list shows `SERVICE MENU`;
  - a second click returns to `Italiano` (wrap-around).

  The existing menu tests that count encoder turns to reach an item are updated where the new item shifts `Ripristino fabb` and `Esci`.

### C-6 — `platformio.ini`, `CHANGELOG.md`: version 2.1.0

- **Satisfies:** REQ-5.1
- **Current state:** `-DFW_VERSION=\"2.0.0\"`; `CHANGELOG.md` has `## [2.0.0] — in collaudo`.
- **Change:** `-DFW_VERSION=\"2.1.0\"`. A new `## [2.1.0]` section with "Added: LCD texts in Italian and English, Lingua/Language item in the Service menu" and, under "To do when upgrading", "v2.0.0 settings are not read: the timer restarts from factory values (Italian language)".
- **Expected behaviour:** the start screen and the Info item show `v2.1.0`.
- **Verification:** `pio run`; the Info test uses the version passed in by the test, so it does not change.

### C-7 — `DOCS-IT.md`, `DOCS.md`, `README.md`, `README-IT.md`, `TECNICO.md`: firmware documentation

- **Satisfies:** REQ-5.2, REQ-5.4
- **Current state:**
  - `DOCS-IT.md` §10 says "I testi sull'LCD sono in italiano" and lists 15 items; the factory values (memory section) do not mention the language. `DOCS.md` has the same content in English.
  - `TECNICO.md` §7 describes the 43-byte, version 1 settings block and the 8-byte `GlobalRecord`; §9 says "15 voci"; the module table has no `Texts`.
  - The READMEs list the Service menu functions.
- **Change:**
  - Manuals: `Lingua/Language` item in the §10 table; "the LCD texts are in Italian or English, chosen in the Lingua/Language item"; Italian among the factory values; an upgrade note from v2.0.0. In `DOCS.md` items are named in their English form, with the Italian in brackets where useful.
  - `TECNICO.md`: §7 with 44 bytes, version 2 and a 9-byte `GlobalRecord` including the language; §9 with 16 items; module table with `Texts.{h,cpp}`.
  - READMEs (`README.md`, `README-IT.md`): "LCD language (Italian/English)" in the Service menu list, and a restyle in the style of the `main` README. At the top the banner `docs/assets/r080-banner.svg` (arrives from `main` with the rebase) and the badges: the branch's `firmware.yml` CI, site, CC BY-NC-SA 4.0 licence, version 2.1.0. Then links to the site pages and the features in compact tables; pinout, build and documents stay.
- **Expected behaviour:** manuals and code describe the same menu.
- **Verification:** reread; `grep -n "in italiano" DOCS-IT.md` no longer finds the old sentence; README preview on GitHub (banner and badges visible, links working).

### C-8 — `site/installazione.html`, `site/flussi.html` on `main`: site

- **Satisfies:** REQ-5.3
- **Current state:** `installazione.html` has one card per Service menu item (`<span class="lcdlabel">&gt;Info</span>` …, labels in Italian even in the English version). `flussi.html` describes the menu paths with the Italian texts.
- **Change:** in a separate PR on `main`, after the firmware merge:
  - a new `>Lingua/Language` card before `>Ripristino fabb`;
  - English LCD labels in the English version of the site (`>Factory reset`, `Confirm?`);
  - in `flussi.html`, the path to change language.
- **Expected behaviour:** the site describes v2.1.0 in both languages.
- **Verification:** green `pages` workflow, pages reachable (HTTP 200), visual check of IT and EN.

## 4. Implementation order

1. **C-1**: `Texts` and the table test. Builds on its own; nothing uses it yet.
2. **C-2**: language in the settings, version 2, with the persistence tests. Depends on C-1 (`LANG_IT`, `LANG_COUNT`).
3. **C-3 + C-4**: `Controller::language()` and main screen. Depends on C-1 and C-2.
4. **C-5**: translated Service menu and `Lingua/Language` item. Depends on C-1 and C-2; the language tests on the main screen also use C-4.
5. **C-6 + C-7**: firmware version and documentation, in the same PR. C-7 needs the banner on `main` and the branch rebased onto `main`.
6. **C-8**: site, in a PR on `main` after the firmware PR is merged.

Each step leaves `pio run` and `pio test -e native` green.

## 5. Technical risks

| Risk | Impact | Mitigation |
|---|---|---|
| A translated text silently cut by `put`/`putF` | Truncated text on the LCD | Per-text limit in `kWidth` and the REQ-4.2 test; for formats the limit accounts for the worst expansion |
| Items shifting in the menu | Existing tests that count turns fail | Update the counts in C-5. Italian texts do not change, so the row comparisons hold |
| Flash pointer used as a RAM string | Unreadable texts only on the Mega (on the PC `PROGMEM` is empty and the test passes) | `text()` is used only with `putF`, `fmemcpy` and `fsnprintf`; visual check on the board during acceptance (#16) |
| Flash and RAM | About 1.7 KB more flash, 0 RAM | Ample room on the Mega; `pio run` reports usage |
| Banner missing on the firmware branch | Broken image in the v2 README | Rebase `feature/arduino-mega-firmware` onto `main` after the `main` README PR, before C-7 |
| Settings lost when upgrading from v2.0.0 | The user redoes the settings | No board has v2.0.0 installed; note in the CHANGELOG |

## 6. Traceability matrix

| REQ | Changes | Verification |
|---|---|---|
| REQ-1.1 | C-1 | Table test: two languages |
| REQ-1.2 | C-1, C-3, C-4, C-5 | Tests of the main screen and menu in English |
| REQ-1.3 | C-1, C-4 | Test: Mode rows and start screen identical in English |
| REQ-1.4 | C-4, C-5 | Existing log tests (`START`, `END`, `CAL`) unchanged with English |
| REQ-1.5 | C-1 | Table test: characters 0x20–0x7E |
| REQ-1.6 | C-1 | Review: Screen and ServiceMenu use only `TextId` and `LANG_COUNT` |
| REQ-1.7 | C-1, C-4, C-5 | Existing `test_timer` tests unchanged |
| REQ-2.1 | C-5 | Test: `>Lingua/Language` before `>Ripristino fabb` |
| REQ-2.2 | C-5 | Test: `Italiano` / `English` in the item |
| REQ-2.3 | C-5 | Test: click and wrap-around |
| REQ-2.4 | C-3, C-4, C-5 | Test: `SERVICE MENU` right after `D` |
| REQ-2.5 | C-5 | Test: confirm beep and block written to EEPROM |
| REQ-3.1 | C-2 | Test: simulated restart with English |
| REQ-3.2 | C-2 | Test: blank EEPROM leads to Italian |
| REQ-3.3 | C-2 | Test: Factory reset after English |
| REQ-3.4 | C-2 | Test: version 1 block leads to factory values |
| REQ-3.5 | C-2 | Test: language 7 with valid CRC |
| REQ-4.1 | C-1 | Table test |
| REQ-4.2 | C-1 | Table test |
| REQ-4.3 | C-1 | Table test |
| REQ-5.1 | C-6 | `pio run`, CHANGELOG |
| REQ-5.2 | C-7 | Reread of the documents |
| REQ-5.4 | C-7 | README preview on GitHub |
| REQ-5.3 | C-8 | `pages` workflow, pages reachable |

## 7. Open questions

- **Q-1 — Review of the text table:** resolved on 2026-10-26, C-1 table approved.
- **Q-2 — Where to press to change the language:** resolved on 2026-10-26, behaviour of C-5.
- **Q-3 — Where the README is updated:** resolved on 2026-10-26, READMEs and manuals in C-7, site in C-8.
