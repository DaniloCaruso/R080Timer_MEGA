# MANAGEMENT — LCD texts in several languages

- **Feature:** `ui-languages`
- **Date:** 2026-10-26
- **Source of the decisions:** grilling session of 2026-10-26; `docs/ui-languages/decisions.md`; `EARS.en.md` and `TECHNICAL.en.md` in this folder.

## Summary

Today the timer's LCD shows a mix of Italian and English: the Service menu is all Italian, the main screen uses English names and Italian abbreviations. With this feature every LCD text is available in Italian and in English, with the language chosen from a new Service menu item. The work is small, touches only the firmware and the documentation, and brings the firmware to v2.1.0. Scope and translations are approved.

## Why

- The repository and the site are public and bilingual: someone who does not speak Italian cannot follow the Service menu today, which holds tests and calibration.
- Before: one fixed, mixed language. After: Italian or English by choice, with the same positions on screen.
- Italian users see no difference: the Italian texts stay as they are today.

## What

**In scope:**
- every LCD text in Italian and in English: main screen, the message after a firmware hang, the whole Service menu;
- a `Lingua/Language` item in the Service menu, with the same name in both languages;
- the language saved with the other settings; Italian as the factory language;
- an automatic check that every translation exists and fits on screen;
- version 2.1.0, with manuals and site updated, and READMEs refreshed with banner and badges.

**Out of scope:**
- languages beyond Italian and English: the structure allows them, but none is needed today;
- accented letters: the display does not have them and the custom symbol slots are almost all taken;
- translating the Mode names (FOCUS, COUNTDOWN, TEST STRIP): they are well-known names, also used on the site and in the manuals;
- the USB log: it stays in English, because it is a diagnostic tool;
- recovering v2.0.0 settings: no board runs it yet.

## How

1. Gather every text in a single table with both translations.
2. Add the language to the saved settings.
3. Make the main screen and the Service menu use the table, with the new Language item.
4. Update version, changelog, manuals and READMEs in the same delivery as the firmware.
5. After integration, update the site in a separate delivery.

## Effort

to be estimated

## Risks

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| A translation too long, cut on screen | Medium | Low | Automatic length check on every change |
| Settings reset when moving to v2.1.0 | Certain | Low | No board runs v2.0.0; note in the changelog |
| Texts unreadable only on the real board | Low | Medium | Visual check during the board acceptance test (#16) |

## Success criteria

- Every LCD text appears in the chosen language (REQ-1.2), except Mode names, start screen and numbers, which are the same in both languages (REQ-1.3).
- In Italian the screen is identical to today (REQ-1.7).
- The language is changed from the Service menu, applies at once and survives power-off (REQ-2.1–2.5, REQ-3.1).
- The factory reset returns to Italian (REQ-3.3).
- Automatic tests block missing, too long or non-displayable translations (REQ-4.1–4.3).
- Firmware v2.1.0 with manuals, READMEs and site updated (REQ-5.1–5.4).

## Decision requested

Scope, text table and open questions approved on 2026-10-26: work can start.

## Open questions

- **Q-1:** resolved, translations approved.
- **Q-2:** resolved, to change the language you enter the item and press.
- **Q-3:** resolved, READMEs and manuals on the firmware branch, only the site on `main`.
