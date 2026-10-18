# EARS — R080 Timer firmware rewrite

- **Feature:** `firmware-rewrite`
- **Date:** 2026-10-18
- **Source of decisions:** grilling session of 2026-10-18 (rounds 1–6); `docs/firmware-rewrite/decisions.md`; `GLOSSARY.md`; code reading at tag `pre-rewrite`.

Terms in italics or with an initial capital (Exposure, Channel, Step, Advance, Base time, Offset, Metronome, Pre-warning, End sound, Service menu, Clock correction, Auto-off, Relay enable, Cancellation) follow `GLOSSARY.md` (which is in Italian).

## Legend

| Pattern | Form |
|---|---|
| Ubiquitous | The system **shall** … |
| Event-driven | **When** `<trigger>`, the system **shall** … |
| State-driven | **While** `<state>`, the system **shall** … |
| Unwanted | **If** `<condition>`, **then** the system **shall** … |
| Optional | **Where** `<feature>`, the system **shall** … |

## REQ-1 — Timing accuracy

### REQ-1.1 (Ubiquitous)
The system **shall** use the ATmega2560 Timer1 in CTC mode with a 1 kHz interrupt as the only source of exposure time.

### REQ-1.2 (Event-driven)
**When** the remaining time of a Countdown reaches zero, the system **shall** switch the relay off within 1 ms of the theoretical edge, measured against the board clock.

### REQ-1.3 (Event-driven)
**When** a test strip with step-by-step Advance reaches a Step, the system **shall** switch the relay off within 1 ms of the theoretical edge of the Step.

### REQ-1.4 (Ubiquitous)
After the Clock correction is applied, the error on the duration of a 60.0 s Exposure measured with an external reference **shall** be at most 2 ms.

### REQ-1.5 (Event-driven)
**When** a test strip reaches a Step, the system **shall** start the step beep within 1 ms of the theoretical edge of the Step.

### REQ-1.6 (State-driven)
**While** a Countdown is running and mute is not active, the system **shall** sound the Metronome at every whole second elapsed since the start of the Exposure, with a period standard deviation below 1 ms over 30 s.

### REQ-1.7 (Ubiquitous)
The system **shall** apply the saved Clock correction to time counting, expressed in parts per million within the range ±1000 ppm.

## REQ-2 — Exposure safety

### REQ-2.1 (Event-driven)
**When** the board powers up or resets for any reason, the system **shall** keep the relay off until the user starts an Exposure.

### REQ-2.2 (Unwanted)
**If** the main loop does not run for more than 500 ms, **then** the system **shall** reset the board through the watchdog, leaving the relay off.

### REQ-2.3 (Unwanted)
**If** the supply voltage drops below the factory brown-out threshold (2.7 V), **then** the system **shall** reset the board, leaving the relay off.

### REQ-2.4 (Ubiquitous)
The system **shall** start or cancel an Exposure only on a main button press that is stable for at least 20 ms.

### REQ-2.5 (Unwanted)
**If** the settings saved in EEPROM have an unknown version or a wrong CRC8, **then** the system **shall** load the factory defaults.

### REQ-2.6 (Unwanted)
**If** the LCD or the I²C bus do not respond, **then** the system **shall** continue Exposures and time counting using only the 7-segment display and record the event in the log.

### REQ-2.7 (Ubiquitous)
Every communication with the LCD **shall** end within a timeout even when the display does not respond, without blocking the main loop.

### REQ-2.8 (Event-driven)
**When** the board powers up again after a power failure that occurred during an Exposure, the system **shall** stay idle with the relay off without resuming the Exposure.

## REQ-3 — Modes and exposures

### REQ-3.1 (Ubiquitous)
The system **shall** offer the Focus, Countdown and Test strip modes, with the behaviour described in the following requirements.

### REQ-3.2 (Event-driven)
**When** the main button is pressed in Focus with the timer stopped, the system **shall** switch the lamp on (if the Channel's Relay enable is active) and show the elapsed time on the 7-segment display.

### REQ-3.3 (Event-driven)
**When** Focus stays on for the maximum time configured in the Service menu (3, 5, 10, 15 or 20 minutes, default 5), the system **shall** perform the Auto-off of the lamp.

### REQ-3.4 (Event-driven)
**When** 10 s remain before the Auto-off, the system **shall** sound an audible warning.

### REQ-3.5 (Event-driven)
**When** the main button is pressed in Countdown with the timer stopped and a set time greater than zero, the system **shall** start an Exposure as long as the set time.

### REQ-3.6 (Event-driven)
**When** the main button is pressed during an Exposure in Countdown or in Focus, the system **shall** perform the Cancellation by switching the relay off and sounding the End sound.

### REQ-3.7 (Event-driven)
**When** a Countdown ends or is cancelled, the system **shall** show the set time on the 7-segment display again, ready to repeat the Exposure.

### REQ-3.8 (Unwanted)
**If** the main button is pressed in Countdown or Test strip with a set time equal to zero, **then** the system **shall** sound an error beep and stay idle.

### REQ-3.9 (Ubiquitous)
In a test strip with precision in Seconds, the system **shall** place the Steps at the cumulative times T, T+P, T+2P, … where T is the Base time and P the precision value.

### REQ-3.10 (Ubiquitous)
In a test strip with F-Stop precision 1/p, the system **shall** place the k-th Step at the cumulative time T·2^(k/p), computed each time from the Base time and rounded to the tenth of a second.

### REQ-3.11 (State-driven)
**While** a test strip with continuous Advance is running, the system **shall** keep the lamp on and sound a step beep at every Step.

### REQ-3.12 (Event-driven)
**When** a test strip with step-by-step Advance reaches a Step, the system **shall** switch the lamp off and wait for a main button press, after which it runs only the increment up to the next Step.

### REQ-3.13 (Event-driven)
**When** a test strip is started after a Cancellation or after it has completed, the system **shall** restart from the first Step.

### REQ-3.14 (Event-driven)
**When** the cumulative time of a test strip reaches 999.9 s, the system **shall** end the test strip by switching the relay off and sounding the End sound.

### REQ-3.15 (Event-driven)
**When** the main button is pressed while the lamp is on during a test strip, the system **shall** perform the Cancellation of the test strip.

### REQ-3.16 (Event-driven)
**When** `D`, `A`, `B` or `C` is pressed or the mode is changed during the pause of a step-by-step test strip, the system **shall** end the test strip by showing the Base time on the 7-segment display, so that the next test restarts from the first Step.

## REQ-4 — Precision and F-Stop

### REQ-4.1 (Ubiquitous)
The system **shall** offer the F-Stop precision values 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64 and the Seconds values 1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20.

### REQ-4.2 (Event-driven)
**When** `*` or `#` is pressed in Countdown or Test strip with precision in Seconds, the system **shall** decrease or increase the set time by the precision value, within the limits 0.1–999.9 s.

### REQ-4.3 (Event-driven)
**When** `*` or `#` is pressed in Countdown or Test strip with F-Stop precision 1/p, the system **shall** decrease or increase the Offset by one and set the time to Base time·2^(Offset/p), rounded to the tenth and limited to 0.1–999.9 s.

### REQ-4.4 (State-driven)
**While** the precision is of F-Stop type, the system **shall** steadily show on line 2 of the LCD the precision value followed by the Offset in sign-and-two-digits format (`+02`, `-01`, ` 00`).

### REQ-4.5 (Event-driven)
**When** the precision encoder is turned with F-Stop precision, the system **shall** recompute the set time from Base time and Offset with the new precision value.

### REQ-4.6 (Event-driven)
**When** the precision encoder button is pressed in Countdown or Test strip, the system **shall** toggle the precision type between Seconds and F-Stop, and on switching to F-Stop set the Base time to the set time with Offset zero.

### REQ-4.7 (Event-driven)
**When** the mode encoder button is pressed in Test strip, the system **shall** toggle the Advance between continuous and step-by-step.

## REQ-5 — Controls

### REQ-5.1 (Event-driven)
**When** a digit is typed in Countdown or Test strip, the system **shall** shift it in from the right of the four-digit set time, shifting the oldest digit out on the left.

### REQ-5.2 (Event-driven)
**When** the first digit is typed after any other action (Exposure, Channel change, `*`/`#`), the system **shall** start a new time instead of appending the digit to the previous time.

### REQ-5.3 (Event-driven)
**When** a time is entered from the keypad, the system **shall** set the Base time to the entered time with Offset zero.

### REQ-5.4 (Event-driven)
**When** `D` is pressed with the timer stopped, the system **shall** reset set time, Base time and Offset of the active Channel to zero.

### REQ-5.5 (Event-driven)
**When** `*` is held for 1 s, the system **shall** toggle mute, confirm it with a short beep and not perform the `*` action on release.

### REQ-5.6 (Event-driven)
**When** `#` is held for 1 s, the system **shall** toggle the Relay enable of the active Channel, confirm it with a short beep and not perform the `#` action on release.

### REQ-5.7 (Event-driven)
**When** `*` or `#` is pressed in Focus, the system **shall** decrease or increase the LCD brightness.

### REQ-5.8 (Event-driven)
**When** `*` or `#` is pressed in Focus while holding `0`, the system **shall** decrease or increase the 7-segment display brightness.

### REQ-5.9 (State-driven)
**While** an Exposure is running, the system **shall** ignore encoders, keypad and Channel change, accepting only the main button.

### REQ-5.10 (Event-driven)
**When** the mode encoder turns by one detent, the system **shall** switch to the next or previous mode relative to the current one.

### REQ-5.11 (Event-driven)
**When** the main button is pressed, the system **shall** react as soon as the press passes the 20 ms debounce, without waiting for release.

### REQ-5.12 (Ubiquitous)
The system **shall** register every key, encoder or button press that passes debounce, even while it updates the displays.

## REQ-6 — Channels and persistence

### REQ-6.1 (Ubiquitous)
Each Channel A, B and C **shall** store mode, precision type and value, Advance, set time, Base time, Offset and Relay enable.

### REQ-6.2 (Event-driven)
**When** `A`, `B` or `C` is pressed with the timer stopped, the system **shall** activate the corresponding Channel, restoring all of its settings.

### REQ-6.3 (Event-driven)
**When** the mode changes, the system **shall** keep the set time of the active Channel.

### REQ-6.4 (Event-driven)
**When** the board powers up, the system **shall** restore the saved active Channel, settings of the three Channels, mute, brightness and Service menu settings.

### REQ-6.5 (Ubiquitous)
The system **shall** save the settings in EEPROM in a format with a version number and CRC8.

### REQ-6.6 (Event-driven)
**When** the firmware starts for the first time on an EEPROM with the previous format, the system **shall** use the factory defaults without attempting migrations.

### REQ-6.7 (Event-driven)
**When** 2 s have passed since the last typed digit or an Exposure starts, the system **shall** save the modified settings in EEPROM.

### REQ-6.8 (Ubiquitous)
The system **shall** write to EEPROM only the bytes whose value has changed.

## REQ-7 — Displays and lights

### REQ-7.1 (Ubiquitous)
The system **shall** always light the LCD in red only.

### REQ-7.2 (Ubiquitous)
The system **shall** manage separate, persistent brightness levels for the LCD and the 7-segment display.

### REQ-7.3 (State-driven)
**While** an Exposure is running, the system **shall** set the LCD to the exposure brightness configured in the Service menu (default: minimum visible) and restore it at the end.

### REQ-7.4 (State-driven)
**While** an Exposure is running, the system **shall** keep the 7-segment display at the brightness chosen by the user.

### REQ-7.5 (Ubiquitous)
The system **shall** keep the LCD layout of the previous version: mode name, precision type and Advance on line 1; precision, Channel, mute icon and relay icon on line 2.

### REQ-7.6 (State-driven)
**While** a test strip is running, the system **shall** show on line 2 of the LCD the current Step number (`St:n`) in place of the precision.

### REQ-7.7 (State-driven)
**While** a test strip is running, the system **shall** show the cumulative time on the 7-segment display.

### REQ-7.8 (State-driven)
**While** a step-by-step test strip is paused between two Steps, the system **shall** show on the 7-segment display the cumulative time of the next Step.

### REQ-7.9 (Event-driven)
**When** the board powers up, the system **shall** show `R080 Timer` and the firmware version for 1.5 s and then the last saved state.

### REQ-7.10 (State-driven)
**While** the mode is Focus, the system **shall** not show any precision on line 2 of the LCD.

### REQ-7.11 (Ubiquitous)
The system **shall** offer 8 brightness levels for the LCD and 8 for the 7-segment display, distributed non-linearly with finer steps at the low levels.

### REQ-7.12 (Ubiquitous)
The factory brightness values **shall** be: 7-segment display at the level equivalent to the value 10 out of 255 of the previous version, LCD at level 3, LCD during the Exposure at level 1.

## REQ-8 — Sounds

### REQ-8.1 (Event-driven)
**When** an Exposure starts, the system **shall** sound a short high-pitched beep.

### REQ-8.2 (State-driven)
**While** the mode is Focus or Test strip, the system **shall** not sound the Metronome.

### REQ-8.3 (Event-driven)
**When** 0.3 s remain before the next Step of a test strip, the system **shall** sound the Pre-warning of three short ticks.

### REQ-8.4 (Event-driven)
**When** an Exposure ends or is cancelled, the system **shall** sound the End sound of three descending tones.

### REQ-8.5 (State-driven)
**While** mute is active, the system **shall** not emit sounds, except for the End sound.

### REQ-8.6 (Ubiquitous)
Start, Metronome, step beep, Pre-warning, End sound, error beep and confirmation beep **shall** be distinguishable from one another by ear.

### REQ-8.7 (Ubiquitous)
The system **shall** emit the sounds with the starting parameters of the following table, gathered in a single table in the code and adjustable during acceptance testing.

| Sound | Parameters |
|---|---|
| Start | 1000 Hz, 60 ms |
| Metronome | 300 Hz, 80 ms |
| Step | 600 Hz, 150 ms |
| Pre-warning | 3 ticks at 800 Hz of 30 ms |
| End sound | 600, 450, 300 Hz of 150 ms with 80 ms pauses |
| Error | 150 Hz, 300 ms |
| Confirmation | 1200 Hz, 40 ms |
| Auto-off warning | 2 tones at 450 Hz of 200 ms |

## REQ-9 — Service menu and diagnostics

### REQ-9.1 (Event-driven)
**When** `D` is held at power-up, the system **shall** open the Service menu.

### REQ-9.2 (Ubiquitous)
The Service menu **shall** show the firmware version and build date.

### REQ-9.3 (Ubiquitous)
The Service menu **shall** show the persistent counters of power-ups, of watchdog resets, of the number of Exposures and of the total lamp-on hours.

### REQ-9.4 (Ubiquitous)
The Service menu **shall** show the maximum main loop duration, the maximum Timer1 ISR duration and the number of Timer1 ticks lost since power-up.

### REQ-9.5 (Ubiquitous)
The Service menu **shall** offer a test for each component: relay, buzzer, 7-segment display, LCD, keypad, encoders and main button.

### REQ-9.6 (Event-driven)
**When** the user starts the calibration log from the Service menu, the system **shall** write its own millisecond count to the USB port, once per second and until exit with `D`.

### REQ-9.7 (Ubiquitous)
The Service menu **shall** allow setting the LCD brightness during the Exposure and the maximum Focus time.

### REQ-9.8 (Event-driven)
**When** the user confirms the factory reset from the Service menu, the system **shall** replace all settings with the factory defaults.

### REQ-9.9 (Event-driven)
**When** the board starts after a watchdog reset, the system **shall** show `RESET: WDT` on the LCD for 3 s and increment the watchdog reset counter.

### REQ-9.10 (Ubiquitous)
The system **shall** write an event log to the USB port at 115200 baud: start-up, start and end of every Exposure with the measured duration, errors.

### REQ-9.11 (Ubiquitous)
The system **shall** count as an Exposure every Countdown and every whole test strip (a test strip counts as one Exposure) and add to the lamp hours all the time with the relay on, Focus included.

### REQ-9.12 (Ubiquitous)
The Service menu **shall** allow entering the Clock correction directly in ppm, or the measured duration of a 600.0 s Countdown from which to compute it.

### REQ-9.13 (Ubiquitous)
The repository **shall** contain the script `tools/calibrate.py`, which compares the calibration log with the PC clock for 30 minutes and prints the Clock correction in ppm.

### REQ-9.14 (Ubiquitous)
The Service menu **shall** use the mode encoder to scroll through the items, its button to enter or confirm, `D` to go back, the numeric keys for values, and a last item "Exit" that restarts the timer in normal use.

## REQ-10 — Platform and hardware

### REQ-10.1 (Ubiquitous)
The system **shall** run on a genuine Arduino Mega 2560 with the current components: Waveshare RGB LCD, serial 7-segment display, 4x4 keypad, two encoders with push button, main button, buzzer and relay.

### REQ-10.2 (Ubiquitous)
The system **shall** drive the 7-segment display through the hardware serial port Serial3 (pin 14).

### REQ-10.3 (Ubiquitous)
The system **shall** use only `Encoder`, `Keypad` and `OneButton` as external libraries, without `EncoderButton`, `LiquidCrystal_I2C`, `ArduinoSTL` and `arduino-timer`.

### REQ-10.4 (Ubiquitous)
The system **shall** keep all wiring unchanged except that of the 7-segment display and the addition of REQ-10.5.

### REQ-10.5 (Ubiquitous)
The wiring **shall** include a 10 kΩ resistor between D47 and GND, which keeps the relay off until the firmware drives the pin.

## REQ-11 — Architecture and quality

### REQ-11.1 (Ubiquitous)
The domain logic **shall** be written in C++ with no Arduino dependencies and communicate with the hardware only through abstract interfaces.

### REQ-11.2 (Ubiquitous)
Each hardware component **shall** be driven by a driver that implements the corresponding domain interface, with `main.cpp` limited to wiring drivers and domain together.

### REQ-11.3 (Ubiquitous)
The domain logic, including the logic run by the ISR, **shall** be covered by automated Unity tests runnable on the PC with `pio test -e native`.

### REQ-11.4 (Event-driven)
**When** a push is made to GitHub, the CI **shall** run `pio run` for the Mega and `pio test -e native` and fail if either of them fails.

### REQ-11.5 (Ubiquitous)
The system **shall** represent time internally in 32-bit milliseconds and use the tenth of a second as the interface unit, with a settable time from 0.1 to 999.9 s.

## REQ-12 — Releases, repository and documentation

### REQ-12.1 (Ubiquitous)
The firmware **shall** follow semantic versioning starting from `v2.0.0` and record every version in `CHANGELOG.md`.

### REQ-12.2 (Event-driven)
**When** a release is published, the project **shall** create a git tag tied to a signed Acceptance checklist.

### REQ-12.3 (Ubiquitous)
At the end of the rewrite the repository **shall** contain a rewritten user manual, `README` and `PINOUT.md`, new technical documents in place of `TECNICO.md` and `DIAGRAMS.md`, no generated PDFs, and `docs/refactoring-timing/` as an archive.

### REQ-12.4 (Ubiquitous)
The repository **shall** use the branches `feature/arduino-mega-firmware` and `feature/esp32-micropython-firmware` and keep the `pre-rewrite` tag on the previous firmware.

### REQ-12.5 (Ubiquitous)
The user manual **shall** describe, as an alternative to `tools/calibrate.py` without a PC, calibration through an audio recording of the start and end beeps of a 600.0 s Countdown.

## Open questions

All the questions raised were resolved in round 7 of the grilling session (2026-10-18).

### Q-1 — Reset cause and brown-out threshold
Resolved: in v1, factory threshold and recognition of the watchdog reset only (REQ-2.3, REQ-9.3, REQ-9.9). Threshold at 4.3 V via ISP in the backlog.

### Q-2 — Sound parameters
Resolved in REQ-8.7.

### Q-3 — Brightness scales
Resolved in REQ-7.11 and REQ-7.12.

### Q-4 — Ending a paused step-by-step test strip
Resolved in REQ-3.16.

### Q-5 — What counts as an Exposure in the counters
Resolved in REQ-9.11.

### Q-6 — Relay during the bootloader
Resolved in REQ-10.5 (preventive resistor; the user believes the relay module already has one).

### Q-7 — Calibration accuracy with a manual stopwatch
Resolved in REQ-9.6, REQ-9.12, REQ-9.13 and REQ-12.5.
