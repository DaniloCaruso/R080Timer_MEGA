# R080 Timer — User manual (firmware v2.1.0)

[Versione italiana](DOCS-IT.md)

R080 Timer drives the enlarger lamp: it decides how long the light exposes the paper. It has three Modes (Focus, Countdown, Test strip), precision in seconds or in fractions of a stop (F-Stop), three Channels with memory and a Service menu for diagnostics and calibration.

Times are shown and entered in tenths of a second, from 0.1 to 999.9 s.

The LCD texts are in Italian; this manual quotes them as they appear, with a translation where useful.

## Contents

1. [Controls](#1-controls)
2. [Displays](#2-displays)
3. [Entering the time](#3-entering-the-time)
4. [Precision: Seconds and F-Stop](#4-precision-seconds-and-f-stop)
5. [Modes](#5-modes)
6. [Channels and memory](#6-channels-and-memory)
7. [Sounds and mute](#7-sounds-and-mute)
8. [Darkroom lights](#8-darkroom-lights)
9. [Power-on and safety](#9-power-on-and-safety)
10. [Service menu](#10-service-menu)
11. [Clock calibration](#11-clock-calibration)
12. [USB log](#12-usb-log)
13. [Quick reference](#13-quick-reference)

---

## 1. Controls

| Control | Action |
|---|---|
| Main button | Starts the Exposure; during the Exposure it cancels it. Acts on press, not on release. |
| Digits `0`–`9` | Enter the time (see §3). `0` is entered when the key is released. |
| `*` / `#` | Decrease / increase the time by the precision (§4). In Focus they set the LCD brightness (§8). |
| `0` held + `*` / `#` | In Focus: 7-segment brightness (§8). |
| `*` held 1 s | Mute on/off; turning it off sounds a confirmation beep (§7). |
| `#` held 1 s | Relay enable of the active Channel, with a confirmation beep (§6). |
| `A` / `B` / `C` | Activate the matching Channel (§6). |
| `D` | Clears set time, Base time and Offset of the active Channel. |
| Mode encoder, turn | Next or previous Mode: Focus, Countdown, Test strip. |
| Mode encoder, press | In Test strip: continuous (`CO`) or step-by-step (`PP`) Advance. |
| Precision encoder, turn | Next or previous precision value (11 values, wrapping). |
| Precision encoder, press | Precision type: Seconds or F-Stop. |

During an Exposure (including Focus with the lamp on) keypad and encoders are ignored: only the main button works.

## 2. Displays

### 7-segment

Shows a time in seconds with one decimal, on four digits: `010.0` = 10.0 s, `999.9` = maximum.

| State | What it shows |
|---|---|
| Idle | The set time of the active Channel |
| Countdown running | The remaining time, rounded up |
| Focus on | The elapsed time; beyond 999.9 s as minutes.seconds (`17.05` = 17 min 05 s) |
| Test strip running | The cumulative time since the start of the test |
| Step-by-step test strip paused | The cumulative time of the next Step |

### LCD

Two rows of 16 characters, backlight always red.

Row 1: Mode, precision type (`[S]` Seconds, `[F]` F-Stop) and, in Test strip, the Advance (`CO` continuous, `PP` step-by-step).

```
FOCUS
COUNTDOWN  [S]
TEST STRIP [F]PP
```

Row 2: precision (columns 0–6), Channel, ♪ icon (sounds on, hidden when muted) and ⚡ icon (relay enabled).

```
 5s      Ch:A ♪⚡     Seconds precision: 5 s
20s      Ch:A ♪⚡     Seconds precision: 20 s
1/3 +02  Ch:A ♪⚡     F-Stop 1/3, Offset +2
1/12-01  Ch:B  ⚡     F-Stop 1/12, Offset -1, muted
St:3     Ch:C ♪⚡     Test strip: Step number 3
         Ch:A ♪⚡     Focus: no precision
```

At power-on the LCD shows ` - R080 Timer - ` and the version for 1.5 s, then the last saved state.

## 3. Entering the time

Digits enter from the right as on a calculator, in tenths of a second, up to four digits; the fifth pushes the oldest one out.

| Keys | Time |
|---|---|
| `5` | 0.5 s |
| `5` `0` | 5.0 s |
| `1` `2` `5` | 12.5 s |
| `1` `2` `3` `4` `5` | 234.5 s |

- The first digit after any other action (Exposure, Channel change, `*`/`#`, encoders…) starts a new time.
- The entered time also becomes the Base time, with Offset zero.
- `D` clears the time. Starting a Countdown or a Test strip with time zero gives the error beep.

## 4. Precision: Seconds and F-Stop

Precision is the step by which `*`/`#` and the Test strip move the time. It has a type and a value:

| Type | Values |
|---|---|
| Seconds | 1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20 s |
| F-Stop | 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64 of a stop |

**Seconds.** `#` adds the precision value to the time, `*` subtracts it, within 0.1–999.9 s.

**F-Stop.** The time is computed from the Base time: time = Base time × 2^(Offset / p), where 1/p is the precision, rounded to the tenth. `#` and `*` move the Offset by one (up to ±99); the Offset stays visible on row 2 (`+02`, `-01`, ` 00`). The step is refused if the time would leave 0.1–999.9 s or if the Base time is zero.

- Pressing the precision encoder switches type: the current time becomes the Base time, with Offset zero.
- Turning the precision encoder in F-Stop recomputes the time from the Base time with the same Offset and the new value (e.g. Offset +2 at 1/3 becomes +2 at 1/2).

Example: time 10.0 s, F-Stop 1/3. `#` three times: 12.6 → 15.9 → 20.0 s (one stop). `*` once from the base time: 7.9 s.

## 5. Modes

### Focus

Lamp on for as long as needed, to compose and focus.

- Main button: the lamp turns on (if the Channel's relay enable is on) and the 7-segment counts the elapsed time. A second press turns it off, with the End sound.
- **Automatic shut-off:** after the maximum time chosen in the Service menu (3, 5, 10, 15 or 20 minutes; factory 5) the lamp turns off by itself, with the End sound. A warning (two tones) sounds 10 s before.
- No Metronome. The set time is not used, but stays in the Channel: digits and the precision encoder (turn and press) have no effect.
- Focus does not count as an Exposure in the counters; its lamp-on time counts in the lamp hours.

### Countdown

A single Exposure as long as the set time.

- Main button: lamp on and countdown on the 7-segment, with the Metronome on every whole second.
- At zero the lamp turns off (by the hardware timer, to the millisecond) and the End sound plays. The 7-segment returns to the set time: another press repeats the same Exposure.
- Press during the Exposure: Cancellation (lamp off, End sound). The next press starts again from the full time. A cancelled Countdown still counts as an Exposure.

### Test strip

A series of cumulative Steps to compare several times on a single strip of paper. The first Step is the set time.

| Precision | Steps (cumulative times) |
|---|---|
| Seconds, value P | T, T+P, T+2P, … with T = set time |
| F-Stop 1/p | Base time × 2^((Offset + k) / p) for k = 0, 1, 2, … |

Every Step is computed from the Base time, so rounding does not accumulate. If two rounded Steps coincide (1/64 with short times), the next one falls 0.1 s later.

Seconds example: T = 5.0 s, P = 5 s → Steps at 5, 10, 15, 20 s… F-Stop 1/2 example with Base time 8.0 s: 8.0 → 11.3 → 16.0 → 22.6 s…

During the test row 2 of the LCD shows the current Step (`St:n`) and the 7-segment the cumulative time. Before each Step the Pre-warning sounds (three ticks at −0.3, −0.2 and −0.1 s), at the Step the step beep. No Metronome.

**Continuous Advance (`CO`).** The lamp stays on from one Step to the next. Cover the strips at each step beep. The main button cancels the test.

**Step-by-step Advance (`PP`).** At each Step the lamp turns off (to the millisecond) and the timer waits: the 7-segment shows the cumulative time of the next Step. Move the paper and press the main button: the lamp turns on again only for the increment up to the next Step.

- While paused, `D` ends the test with the End sound. `A`, `B`, `C` and a turn of the Mode encoder end it too, and also switch to the pressed Channel or to the next or previous Mode.
- With the lamp on, the main button cancels the test.

**End of the test.** The test goes on until it is cancelled or ended; at 999.9 s cumulative it ends by itself. Every new test starts again from the first Step. A whole test counts as one Exposure, even when cancelled.

## 6. Channels and memory

Keys `A`, `B` and `C` recall three Channels. Each Channel is a complete print setting: Mode, precision type and value, Advance, set time, Base time, Offset and relay enable. Changing Channel restores everything; changing Mode does not change the time.

**Relay enable.** Holding `#` for 1 s toggles, for the active Channel, whether the Exposure really turns on the lamp (⚡ icon) or only runs the time, useful to try a cycle with the lamp off.

**Memory.** Settings are saved automatically 2 s after the last change and at the start of every Exposure: active Channel, the three Channels, mute, brightness and Service menu settings. At power-on everything is as it was.

**Factory values.** For each Channel: Countdown, Seconds 1 s, continuous Advance, 10.0 s, relay enabled. Global: Channel A, mute off, LCD level 3, 7-segment level 3, LCD during exposure level 1, Focus maximum 5 minutes, Italian language, clock correction 0 ppm.

At the first power-on with v2.1.0 the settings of the previous versions (v1 and v2.0.0) are not read: the timer starts from factory values.

## 7. Sounds and mute

| Sound | When | How it sounds |
|---|---|---|
| Start | Exposure start or step-by-step resume | short high beep (1000 Hz, 60 ms) |
| Metronome | Every second of a Countdown | low tick (300 Hz, 80 ms) |
| Pre-warning | 0.3 s before each Step | three short ticks (800 Hz, 30 ms) |
| Step | At each Test strip Step | medium beep (600 Hz, 150 ms) |
| End sound | End or Cancellation | three falling tones (600, 450, 300 Hz) |
| Focus warning | 10 s before automatic shut-off | two tones (450 Hz, 200 ms) |
| Error | Start with time zero; value refused in the Service menu | long low tone (150 Hz, 300 ms) |
| Confirm | Mute, relay enable, value saved in the Service menu | very high beep (1200 Hz, 40 ms) |

**Mute.** Hold `*` for 1 s to turn it on or off; turning it on stays silent, turning it off sounds the confirmation. While muted only the End sound plays, so the end of an Exposure is never missed; the Service menu sounds stay audible too.

## 8. Darkroom lights

- The LCD is always and only red. 8 brightness levels for the LCD and 8 for the 7-segment, denser at the low end.
- In Focus, lamp off: `*`/`#` dim/brighten the LCD; holding `0`, `*`/`#` dim/brighten the 7-segment.
- During every Exposure (including Focus on) the LCD drops to the "LCD exposure" level of the Service menu (factory 1, the minimum) and returns to the normal level at the end. The 7-segment stays at the chosen level.

## 9. Power-on and safety

- At power-on and after every reset the relay stays off until an Exposure is started. An Exposure interrupted by a power cut is never resumed.
- The main button acts only after 20 ms of stable press: a bounce never starts or cancels an Exposure.
- **Watchdog:** if the firmware hangs, the lamp turns off within 250 ms and the board restarts after 500 ms. On restart the LCD shows `RESET: WDT` for 3 s and the watchdog reset counter increases.
- Below 2.7 V supply the board resets (brown-out) with the relay off.
- If the LCD fails or is disconnected the timer keeps working with the 7-segment only; the LCD is brought back automatically as soon as it answers.

## 10. Service menu

Opens by holding `D` while powering on the timer (until the start screen). No Exposures can be started in the menu. The LCD texts are in Italian or English, chosen in the Lingua/Language item (factory: Italian); below, item names are in English with the Italian in brackets.

| Control | Action |
|---|---|
| Mode encoder, turn | Scrolls the items; inside an item, changes screen |
| Mode encoder, press | Enters the item; inside an item, confirms and saves the value |
| `D` | Back to the item list without saving |
| Digits | Enter the value, calculator style |
| `*` | Changes the sign of the value (after the digits) |

An out-of-range value gives the error beep and must be entered again; an accepted one gives the confirm beep and returns to the list.

| Item (LCD) | Content |
|---|---|
| Info | Firmware version and build date |
| Counters (Contatori) | Four screens: Power-ons (every start-up, watchdog resets included), Watchdog resets, Exposures (Countdowns and test strips, cancelled ones included), Lamp hours (relay-on time, Focus included) |
| Health (Salute) | Three screens: Loop max (µs), ISR max (µs), Lost ticks (Timer1 ticks lost since power-on) |
| Relay test (Test rele) | A press turns the relay on and off; leaving turns it off |
| Buzzer test (Test buzzer) | Turning picks one of the 8 sounds, pressing plays it (1 end, 2 error, 3 Focus warning, 4 start, 5 step, 6 pre-warning, 7 metronome, 8 confirm) |
| 7-segment test (Test 7 segmenti) | Lights all segments (`888.8`) |
| LCD test (Test LCD) | Fills every cell |
| Keypad test (Test tastierino) | Shows the last key pressed; `D` is tested by leaving |
| Encoder test (Test encoder) | `M` and `P`: position and press count of the two encoders |
| Button test (Test pulsante) | Press, Pressed, Released |
| Calibration (Calibrazione) | Three screens: Calibration log, PPM correction, Measured 600 s (see §11) |
| LCD exposure (LCD esposizione) | LCD level during the Exposure, 1 to 8 |
| Focus max time (Tempo max Focus) | Minutes before automatic shut-off: 3, 5, 10, 15 or 20 |
| Lingua/Language | Shows the LCD language (`Italiano` or `English`); each press switches to the other language and saves it at once; `D` returns to the list, already in the new language. The item has the same name in both languages |
| Factory reset (Ripristino fabb) | Inside the item, a press asks for confirmation and a second press restores all settings to factory values, clock correction and language included (counters are kept); `D` cancels |
| Exit (Esci) | Back to normal use |

In the tests every control goes to the component under test; `D` leaves.

## 11. Clock calibration

The board's crystal can be off by a few tens of parts per million (ppm): 50 ppm is 3 ms over 60 s. The clock correction compensates for this offset and is saved with the settings. It needs measuring once; after a factory reset it returns to 0.

### Main method: script `tools/calibrate.py`

You need a PC with Python 3.10 or later and `pyserial` (`pip install pyserial`), and the board's USB cable.

1. Close any program using the serial port (including `pio device monitor`).
2. On the timer: Service menu → Calibration → "Calibration log" screen, press the encoder. The LCD shows `Active, D: exit` (in Italian `Attivo, D: esci`).
3. On the PC: `python3 tools/calibrate.py /dev/ttyACM0` (on Windows e.g. `COM3`).
4. After 30 minutes the script prints, for example, `Correzione del clock: +23 ppm`. The value is absolute: it does not depend on the correction already saved.
5. On the timer: `D` closes the log and returns to the list. Press the encoder to enter Calibration again, turn to the "PPM correction" screen, type the value (`2` `3`), press `*` if it is negative, press the encoder to save.

Options: `--minutes N` changes the measurement length; `--self-test` checks the script without a board. If the timer restarts when the port opens (the start screen appears again), stop the script with Ctrl+C, open the Service menu again and restart from step 2.

### Without a PC: audio recording of a 600.0 s Countdown

You need an audio recorder (a smartphone is fine) and a program that shows the waveform (for example Audacity).

1. Turn mute off (♪ icon visible): the start beep does not sound when muted. You can turn the relay enable off to keep the lamp dark.
2. Countdown, time `6` `0` `0` `0` (600.0 s).
3. Start recording near the timer, press the main button and wait for the End sound.
4. On the waveform measure the time from the start of the start beep to the start of the first tone of the End sound. The Metronome ticks, one per second, help to find your way.
5. Restart the timer holding `D`: Service menu → Calibration, turn to the "Measured 600 s" screen. Type the measured duration in hundredths of a second (`6` `0` `0` `1` `2` for 600.12 s) and press the encoder.

The timer computes the new correction starting from the saved one, so the measurement can be repeated to refine it. A 10 ms reading error on the waveform is about 17 ppm: the script is more accurate.

## 12. USB log

With the board connected to a PC (115200 baud, for example `pio device monitor -b 115200`) the timer writes one line per event, prefixed by the milliseconds since power-on:

```
t=12 BOOT reset=POWER
t=8301 START mode=CD ch=A ms=100000
t=108305 END ms=100000 measured=100000
```

Events: `BOOT` (with cause `POWER` or `WDT`), `ERROR settings=factory`, `START` (Mode `FO`, `CD`, `TS`, Channel, requested length), `END` and `CANCEL` (with the length measured by the timer), `LCD offline` / `LCD online`, `CAL` (calibration log), `LOST lines=n` (lines dropped because the PC was not reading).

## 13. Quick reference

| I want to… | I do… |
|---|---|
| Expose for 12.5 s | Countdown, `1` `2` `5`, main button |
| Repeat the last Exposure | Main button |
| Cancel an Exposure | Main button |
| One third of a stop more | F-Stop 1/3, `#` |
| A step-by-step strip | Test strip, press the Mode encoder until `PP` |
| Silence the timer | `*` held 1 s |
| Expose without turning the lamp on | `#` held 1 s (⚡ disappears) |
| Dim the lights | Focus with lamp off: `*` (LCD), `0` + `*` (7-segment) |
| Open the Service menu | `D` held at power-on |
