# EARS — LCD texts in several languages

- **Feature:** `ui-languages`
- **Date:** 2026-10-26
- **Source of the decisions:** grilling session of 2026-10-26 (questions Q1–Q12); `docs/ui-languages/decisions.md`; reading of the code on `feature/arduino-mega-firmware` (v2.0.0): `Screen`, `ServiceMenu`, `Settings`, `Controller`, native tests.

Capitalised terms (Mode, Channel, Step, Advance, Service menu, Factory reset, Exposure) follow `GLOSSARY.md`. These requirements extend those in `docs/firmware-rewrite/requirements-docs/EARS.en.md`.

## Legend

| Pattern | Form |
|---|---|
| Ubiquitous | The system **shall** … |
| Event-driven | **When** `<trigger>`, the system **shall** … |
| State-driven | **While** `<state>`, the system **shall** … |
| Unwanted | **If** `<condition>`, **then** the system **shall** … |
| Optional | **Where** `<feature>`, the system **shall** … |

## REQ-1 — Languages and texts

### REQ-1.1 (Ubiquitous)
The system **shall** offer two languages for the LCD texts: Italian and English.

### REQ-1.2 (State-driven)
**While** a language is selected, the system **shall** show every LCD text in that language: test strip Advance, Channel and Step on the main screen, the watchdog reset message, and in the Service menu the title, items, screen titles, hints, confirmations and tests.

### REQ-1.3 (Ubiquitous)
The system **shall** show the Mode names (`FOCUS`, `COUNTDOWN`, `TEST STRIP`), the start screen, the units (`s`, `us`, `ppm`) and the numbers identically in every language.

### REQ-1.4 (Ubiquitous)
The system **shall** write the USB event log in English only, whatever language is selected.

### REQ-1.5 (Ubiquitous)
Every LCD text **shall** use only the printable ASCII characters 0x20 to 0x7E, with no new custom characters.

### REQ-1.6 (Ubiquitous)
Adding a language **shall** require only the texts of the new language and its name, with no change to the logic of the main screen and the Service menu.

### REQ-1.7 (State-driven)
**While** the language is Italian, the system **shall** show the same LCD texts as v2.0.0.

## REQ-2 — Choosing the language

### REQ-2.1 (Ubiquitous)
The Service menu **shall** offer at its first level, right before the Factory reset, the item `Lingua/Language`, with the same name in every language.

### REQ-2.2 (State-driven)
**While** the `Lingua/Language` item is open, the system **shall** show the name of the selected language written in that language (`Italiano`, `English`).

### REQ-2.3 (Event-driven)
**When** the user presses the Mode encoder button in the `Lingua/Language` item, the system **shall** select the next language, wrapping to the first after the last.

### REQ-2.4 (Event-driven)
**When** the language changes, the system **shall** show all following LCD texts in the new language without restarting the board.

### REQ-2.5 (Event-driven)
**When** the language changes, the system **shall** play the confirm beep and save the settings at once, as for the other Service menu values.

## REQ-3 — Persistence and factory values

### REQ-3.1 (Event-driven)
**When** the board powers on, the system **shall** restore the saved language together with the other settings.

### REQ-3.2 (Ubiquitous)
The factory value of the language **shall** be Italian.

### REQ-3.3 (Event-driven)
**When** the user confirms the Factory reset, the system **shall** also set the language back to Italian.

### REQ-3.4 (Event-driven)
**When** the firmware starts on an EEPROM holding a version 1 settings block, the system **shall** use the factory values without attempting any migration.

### REQ-3.5 (Unwanted)
**If** the settings block is valid but the saved language is not one of those offered, **then** the system **shall** use Italian and keep all the other saved settings.

## REQ-4 — Verification

### REQ-4.1 (Ubiquitous)
The `pio test -e native` suite **shall** fail if an LCD text is missing or empty in any language.

### REQ-4.2 (Ubiquitous)
The `pio test -e native` suite **shall** fail if an LCD text exceeds, in any language, the number of columns available at its position.

### REQ-4.3 (Ubiquitous)
The `pio test -e native` suite **shall** fail if an LCD text contains a character outside 0x20–0x7E.

## REQ-5 — Version and documentation

### REQ-5.1 (Ubiquitous)
The firmware with this feature **shall** have version `2.1.0` and an entry in `CHANGELOG.md` noting the return to factory values on upgrade.

### REQ-5.2 (Ubiquitous)
The user manual (`DOCS-IT.md`, `DOCS.md`), the READMEs and `TECNICO.md` **shall** describe the `Lingua/Language` item, the factory language and the new settings block format.

### REQ-5.3 (Event-driven)
**When** firmware v2.1.0 is integrated into `feature/arduino-mega-firmware`, the site on `main` **shall** describe the `Lingua/Language` item and the LCD texts in both languages.

### REQ-5.4 (Ubiquitous)
The v2 READMEs (`README.md`, `README-IT.md`) **shall** open with the project banner, the CI, site, licence and version badges and links to the site pages, in the style of the `main` README.

## Open questions

### Q-1 — Review of the text table
Resolved on 2026-10-26: the TECHNICAL C-1 table is approved as is (REQ-1.2, REQ-1.7).

### Q-2 — Where to press to change the language
Resolved on 2026-10-26: you enter the item and each click changes the language while staying inside (REQ-2.2, REQ-2.3).

### Q-3 — Where the README is updated
Resolved on 2026-10-26: READMEs and manuals are updated in the firmware PR, only the site on `main` (REQ-5.2, REQ-5.3).
