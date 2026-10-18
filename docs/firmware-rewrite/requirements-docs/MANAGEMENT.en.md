# MANAGEMENT — R080 Timer firmware rewrite

- **Feature:** `firmware-rewrite`
- **Date:** 2026-10-18
- **Source of decisions:** grilling session of 2026-10-18; `docs/firmware-rewrite/decisions.md`; requirements in `EARS.en.md`; technical detail in `TECHNICAL.en.md`.

## Summary

The firmware of the enlarger timer must be rewritten from scratch, keeping the same functions for the person using it in the darkroom. The current version compiles but has never been tried on the board, and the code review found defects that make it unusable: a beep that plays forever while the timer is stopped, a display that always shows zero, channel memories that get lost, buttons that do not respond. The rewrite aims at a reliable product: exposures accurate to the thousandth of a second, a lamp that never stays on by mistake, controls that always respond, and logic verified by automated tests. All the open questions have been decided: you are asked to approve the scope and to authorize the start of implementation.

## Why

- **Today the timer is not usable.** While stopped, the buzzer repeats the end sound without pause, and the seconds display always shows zero, so the time being typed cannot be seen.
- **Settings get lost.** The time saved for each channel is reset at power-on and at every mode change.
- **Controls are slow.** The program takes about a tenth of a second per cycle, because of needless waits in the display handling: short presses of the start button get lost.
- **No verification.** The documents from the previous work claim goals were met "by construction", but no test on the board was done and some claims do not match the code.
- **No safety net.** If the program freezes during an exposure, nothing turns the lamp off.

After the rewrite: maximum error of 2 thousandths of a second on a 60-second exposure, lamp turned off automatically within half a second if the program freezes, controls that respond when pressed.

## What

**In scope**
- Today's functions: focus, single exposure, test strip (continuous and step-by-step), adjustment in seconds or in fractions of a stop, three print memories, mute, lamp exclusion per channel.
- Each memory saves the whole print setting, and at power-on everything is found as it was.
- Safety: lamp always off at power-on and in case of a fault, automatic reset if the program stops, settings protected against data corruption, automatic switch-off of focus after a configurable time.
- Use in the dark: display always red, brightness adjustable separately, display dimmed during the exposure, sounds that are easy to tell apart.
- Service menu: clock calibration (with a script on the computer or, without a computer, by recording the sound of the beeps), diagnostics, test of each component, return to factory settings, event log readable from the computer.
- Automated tests on the computer and automatic check at every code change.
- Two small wiring changes: the seconds display cable moved to another pin, to remove the last source of delay, and a resistor on the relay pin that keeps the lamp off at power-on.

**Out of scope (in the backlog)**
- Stronger protection against voltage drops: it needs an external programmer, to be used only once.
- Pause and resume of the exposure, dry-down compensation, saved programs, foot pedal, safelight control: these are new functions and will be considered only once the base is stable.
- Resuming an exposure interrupted by a power cut: excluded for good, because at power-on the paper may have moved.
- The ESP32 version follows a separate path.

## How

1. **Preparation:** renaming of the branches, saving the current version with a label, an environment for tests on the computer and an automatic check on GitHub.
2. **Domain logic:** the timer's behavior is written without depending on the board and verified by automated tests before touching the hardware.
3. **Connection to the hardware:** thin components for relay, displays, keypad, encoders, buzzer, memory and safety.
4. **Replacement:** the new firmware takes the place of the old one in a single step.
5. **Acceptance and release:** tests on the board following a signed checklist, updated documentation, release of version 2.0.0.

## Effort

*to be estimated*: the interview did not set an estimate.

## Risks

| Risk | Probability | Impact | Mitigation |
|---|---|---|---|
| With the factory protection, a moderate voltage drop can make the board behave unpredictably and is not recognized | Low | Medium | Accepted for the first version; stronger protection with an external programmer in the backlog |
| The lamp turns on for an instant at power-on | Low | High | Preventive resistor on the relay pin |
| Inaccurate clock calibration | Low | Medium | Automatic 30-minute measurement with the computer instead of a hand stopwatch |
| A complete rewrite introduces new defects | Medium | High | Automated tests on all the logic, a signed acceptance checklist for every release, previous version always recoverable |
| Not enough memory on the board | Low | Medium | Today 18% of the RAM is used; checked at every build |
| Wear of the counter memory after about 100,000 exposures | Low | Low | Acceptable for the first version |

## Success criteria

- 60-second exposure with a maximum error of 2 thousandths of a second after calibration (REQ-1.4).
- Lamp turned off within 1 thousandth of a second of the end of the exposure, also in the step-by-step test strip (REQ-1.2, REQ-1.3).
- Steady metronome, with a variation below 1 thousandth of a second (REQ-1.6).
- Lamp always off at power-on and after any reset, no exposure resumed after an interruption (REQ-2.1, REQ-2.8).
- Automatic reset within half a second if the program freezes (REQ-2.2).
- A faulty display does not interrupt the exposure (REQ-2.6).
- No key press lost; immediate start when the button is pressed (REQ-5.11, REQ-5.12).
- Each channel finds its complete setting again, even after power-off (REQ-6.1, REQ-6.4).
- Display always red and dimmed during the exposure (REQ-7.1, REQ-7.3).
- All the logic verified by automated tests at every change (REQ-11.3, REQ-11.4).
- Release 2.0.0 with a signed acceptance checklist (REQ-12.1, REQ-12.2).

## Decision requested

Approve the scope described and authorize the start of implementation in the order given. Alternatively, indicate the parts of the scope to change.

## Open questions

None. The seven questions that came up were decided in the last round of questions:

- **Q-1 — Cause of a reset due to a voltage drop:** for now the factory protection is kept and only program freezes are recognized; the stronger protection goes into the backlog.
- **Q-2 — Sounds:** tone and length of each sound are set, to be fine-tuned by ear during acceptance testing.
- **Q-3 — Brightness:** 8 levels per display, closer together at low brightness; the seconds display starts from today's brightness.
- **Q-4 — End of the step-by-step test strip:** the D key closes a test strip paused between two steps, as does a change of channel or mode.
- **Q-5 — Exposure count:** the single exposure and the test strip (once per test strip) count; focus counts only in the lamp hours.
- **Q-6 — Lamp at power-on:** a preventive resistor is fitted on the relay pin.
- **Q-7 — Calibration method:** a script on the computer as the main method, audio recording as the alternative.
