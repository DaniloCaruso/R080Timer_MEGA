# R080 TIMER MEGA - User Manual & Technical Documentation

## Table of Contents

1. [Overview](#overview)
2. [Hardware](#hardware)
3. [Controls](#controls)
4. [Display](#display)
5. [Operating Modes](#operating-modes)
6. [F-Stop Navigation](#f-stop-navigation)
7. [Channels](#channels)
8. [EEPROM Persistence](#eeprom-persistence)
9. [Sound](#sound)
10. [Software Architecture](#software-architecture)
11. [Quick Command Reference](#quick-command-reference)

---

## Overview

R080 Timer MEGA is a precision digital timer for darkroom photographic enlargers. Based on Arduino Mega 2560, it offers three operating modes (Focus, Countdown, Test Strip), F-Stop navigation, 3 independent channels with memory, and settings persistence.

**Internal time unit:** deciseconds (1/10 second). The value `100` on the display equals `10.0` seconds.

---

## Hardware

| Component                 | Connection                          |
|--------------------------|--------------------------------------|
| Arduino Mega 2560        | Main board                           |
| LCD RGB 16x2 (Waveshare) | I2C (addr 0x3E/0x60)               |
| 7-Segment Display         | SoftwareSerial on pin A2 (9600 baud)|
| 4x4 Keypad               | Cols: 41, 39, 37, 35 / Rows: 33, 31, 29, 27 |
| Mode Encoder              | DT=3, CLK=4, button SW=17          |
| Precision Encoder         | DT=18, CLK=19, button SW=16        |
| Main Button               | Pin 2 (INPUT_PULLUP)               |
| Buzzer                    | Pin 49                              |
| Enlarger Relay            | Pin 47                              |

---

## Controls

### Main Button (pin 2)

- **Single press:** Start/Stop the current operation (toggle).
- When started: relay activates (if enabled for current channel) and timer runs.
- When stopped: relay deactivates and timer stops.

### Mode Encoder (left)

- **Rotation:** Cycles through 3 modes: FOCUS -> COUNTDOWN -> TEST STRIP -> FOCUS...
- **Button press:** In TEST STRIP, toggles advancement mode:
  - `CO` = Continuous (consecutive exposures without pause)
  - `PP` = Step-by-Step (stops at each step, press again to continue)

### Precision Encoder (right)

- **Rotation:** Changes precision value among 11 levels.
  - In F-Stop: 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64
  - In Seconds: 1s, 2s, 3s, 4s, 5s, 6s, 7s, 8s, 10s, 15s, 20s
- **Button press:** Toggles precision type between F-Stop (`F`) and Seconds (`S`).

### 4x4 Keypad

Physical layout:

```
 1  2  3  A
 4  5  6  B
 7  8  9  C
 *  0  #  D
```

---

## Display

### 7-Segment Display

Shows current time in deciseconds with 4 digits and a decimal point on the first digit.

Examples:
- `10.00` = 100.0 seconds (1000 deciseconds)
- ` 5.00` = 50.0 seconds
- ` 0.30` = 3.0 seconds

### LCD 16x2

Layout depends on the active mode:

**FOCUS:**
```
Row 0: FOCUS
Row 1:          Ch:A ♪⚡
```
No precision shown. `♪` = sound on, `⚡` = relay enabled.

**COUNTDOWN:**
```
Row 0: COUNTDOWN  [S]
Row 1: Pr: 5s    Ch:A ♪⚡
```
`[S]` = Seconds type, `[F]` = F-Stop type.

**COUNTDOWN in F-Stop:**
```
Row 0: COUNTDOWN  [F]
Row 1: Pr:1/3+02 Ch:A ♪⚡
```
`+02` = 2 stops above base time. `-01` = 1 stop below. ` 00` = at base time.

**TEST STRIP:**
```
Row 0: TEST STRIP [S] CO
Row 1: Pr: 5s    Ch:B ♪⚡
```
`CO` = Continuous, `PP` = Step-by-Step.

---

## Operating Modes

### 1. FOCUS

**Purpose:** Continuous projection for focusing and composition.

**Behavior:**
- Pressing the button: relay activates and timer counts upward from 0 (increments every 0.1s).
- The displayed time grows continuously.
- Pressing again: relay deactivates and timer stops.

**Special controls in FOCUS:**
- `*` key = decrease 7-segment display brightness (from 10 to 1)
- `#` key = increase 7-segment display brightness (from 1 to 10)
- Number keys are disabled.

### 2. COUNTDOWN

**Purpose:** Timed exposure with countdown.

**Preparation:**
1. Type desired time with the numeric keypad (up to 4 digits, in deciseconds).
   - Example: `1` `0` `0` = 10.0 seconds
   - Example: `5` = 0.5 seconds
   - Example: `5` `0` = 5.0 seconds
2. Or use `*`/`#` to navigate time (see F-Stop or Seconds section).

**Behavior:**
- Pressing the button: relay activates, timer counts down.
- **Metronome:** every second (every 10 deciseconds) a beep is emitted.
- At zero: relay deactivates, stop sound (3 beeps), timer stops automatically.
- **Repeat last exposure:** at completion, the time is restored to the starting value. You can immediately restart by pressing the button.
- Pressing the button during countdown: interrupts the exposure.

**Time navigation:**

With **Seconds** type (`S`):
- `#` = adds `precision * 10` deciseconds to time (e.g. precision 5s -> +50 deciseconds)
- `*` = subtracts `precision * 10` deciseconds (minimum 1)

With **F-Stop** type (`F`):
- `#` = move up one stop (see F-Stop section)
- `*` = move down one stop
- Display shows offset from base time (e.g. `+02`, `-01`, ` 00`)

### 3. TEST STRIP

**Purpose:** Incremental exposures for exposure test strips.

**Preparation:**
1. Set base time with the keypad (first exposure step).
2. Choose precision (distance between steps).
3. Choose mode: Continuous (`CO`) or Step-by-Step (`PP`).

**Behavior - Continuous (`CO`):**
- Pressing the button: relay activates, timer counts upward.
- At each step a beep is emitted. 3 deciseconds before the step: warning beep.
- Timer automatically continues to the next step.
- Pressing the button: stops everything.

**Behavior - Step-by-Step (`PP`):**
- Pressing the button: relay activates, timer counts to first step.
- At step: relay deactivates, timer stops, display shows next target.
- User can move the paper strip under the enlarger.
- Pressing again: resumes for next step (timer continues from where it stopped).

**Step calculation:**

With **Seconds** type: linear steps. E.g. with precision 5s and base 50 deciseconds (5s):
- Step 1: 0 -> 50 (5.0s)
- Step 2: 50 -> 100 (10.0s)
- Step 3: 100 -> 150 (15.0s)

With **F-Stop** type: logarithmic steps. E.g. with precision 1/3 and base 50 deciseconds:
- Step 1: 0 -> 50
- Step 2: 50 -> 63 (50 * 2^(1/3) = 63)
- Step 3: 63 -> 79 (63 * 2^(1/3) = 79)

---

## F-Stop Navigation

### Concept

F-Stop is a logarithmic system used in photography. Doubling the time equals +1 stop. The timer supports fractional stops for fine adjustments.

### How it works

1. **Set a base time** by typing it with the keypad (e.g. `100` = 10 seconds).
2. **Press the precision encoder button** to switch to F-Stop mode (`F`).
3. **Rotate the precision encoder** to choose the fraction: 1/1 (full stops), 1/2 (half stops), 1/3 (third stops), etc.
4. **Press `#`** to go up one stop. **Press `*`** to go down.

### Practical example

Base time: 100 deciseconds (10.0s), precision 1/3 stop:

| Key | Offset | Calculation | Resulting time |
|-----|--------|-------------|---------------|
| (base)| ` 00` | 100         | 10.0s         |
| `#`   | `+01` | 100 * 2^(1/3) = 126 | 12.6s  |
| `#`   | `+02` | 100 * 2^(2/3) = 159 | 15.9s  |
| `#`   | `+03` | 100 * 2^(3/3) = 200 | 20.0s  |
| `*`   | `+02` | 100 * 2^(2/3) = 159 | 15.9s  |

Formula: `time = baseTime * 2^(offset / precision)`

### Important notes

- Base time is set automatically when you type digits on the keypad.
- Switching channels updates the base time to the new channel's time.
- Offset is shown on LCD as `+02`, `-01`, ` 00`.
- Calculated time is always between 1 and 9999 deciseconds.
- Changing precision (e.g. from 1/3 to 1/6) recalculates time with the same offset but new precision value.

---

## Channels

The timer has **3 independent channels** (A, B, C), each with:
- Its own stored time
- Its own relay state (enabled/disabled)

### Commands

| Key | Action |
|-----|--------|
| `A` | Select channel A |
| `B` | Select channel B |
| `C` | Select channel C |
| `D` | Reset current channel (time to 0, F-stop reset) |

### Behavior

- When switching channels, the previous channel's time is saved automatically.
- The new channel's time is loaded onto the display.
- F-stop state (base time and offset) is reset to the new channel's time.
- Settings are saved to EEPROM on every change.

### Per-channel relay

Each channel has an independent relay. Useful if you have multiple enlargers or want to disable the relay for a specific channel.

- **Hold `#`** = toggle current channel's relay (ON/OFF)
- The `⚡` icon on LCD indicates relay enabled; blank space = disabled.
- If relay is disabled, the timer runs normally but the relay doesn't activate.

---

## EEPROM Persistence

Settings are automatically saved to Arduino's EEPROM and restored at power-on.

### Stored data (13 bytes)

| Address | Data | Type |
|---------|------|------|
| 0 | Magic byte (0xA5) | byte |
| 1 | Display brightness (1-10) | byte |
| 2 | Mute (0/1) | byte |
| 3 | Active channel (1-3) | byte |
| 4-5 | Channel A time | int16 |
| 6-7 | Channel B time | int16 |
| 8-9 | Channel C time | int16 |
| 10 | Channel A relay | byte |
| 11 | Channel B relay | byte |
| 12 | Channel C relay | byte |

### When it saves

- Channel switch (A, B, C)
- Time modification (digits, `*`/`#`, F-stop)
- Channel reset (D)
- Mute toggle (hold `*`)
- Relay toggle (hold `#`)
- Brightness change (in FOCUS: `*`/`#`)

### Technical notes

- Uses `eeprom_update_*` which writes ONLY if the value differs from what's in memory (preserves EEPROM life, ~100,000 write cycles per cell).
- On first boot (virgin EEPROM), the magic byte won't be present and default values will be used.

---

## Sound

### Tones

| Event | Frequency | Duration |
|-------|-----------|----------|
| Metronome (countdown) | 300 Hz | 80ms, every second |
| Warning beep (test strip) | 600 Hz | 50ms, 3 deciseconds before step |
| Step beep (test strip) | 600 Hz | 150ms, at step target |
| Stop sound (countdown end) | 300 Hz | sequence: 75ms, pause, 75ms, pause, 150ms |
| Unmute confirmation | 600 Hz | 200ms |

### Mute control

- **Hold `*`** = toggle mute/unmute
- `♪` on LCD = sound active
- Blank space = muted

### Non-blocking system

The stop sound uses a state machine (`SoundPlayer`) that plays the beep sequence without blocking the main loop. The `tickSound()` function is called on every loop iteration.

---

## Software Architecture

### File structure

```
src/main.cpp                 -> setup() + loop()
include/
  types.h                    -> Enums, Operation/OperationStatus structs
  constants.h                -> Pins, tones, texts, keypad layout
  variables.h                -> Globals, hardware objects, Timer<3>
  functions/
    base.h                   -> relayStart(), noop(), isInError()
    config.h                 -> Hardware initialization, resetTime()
    fstop.h                  -> FStopState: F-stop calculation
    storage.h                -> EEPROM: loadSettings(), saveSettings()
    presentation.h           -> LCD & 7-segment: showTime(), showChannel(), etc.
    sound.h                  -> Non-blocking SoundPlayer, emitSound(), tickSound()
    timer_manager.h          -> Timer callbacks: focus, countdown, test strip
    actions.h                -> focus(), countdown(), testStrip()
    operations.h             -> initializeOperations(), setOperationStatus()
    keypad_mgmt.h            -> evaluateKey(), switchToChannel(), F-stop nav
    encoder_mgmt.h           -> evaluateEncoderMod(), evaluateEncoderPrec()
    running_mgmt.h           -> manageMainButtonAction(), configureMainButton()
```

### Main loop

```
loop() {
  mainButton.tick()        -> Button handling (OneButton, debounce)
  appTimer.tick()          -> Timer callback execution (arduino-timer)
  tickSound()              -> Non-blocking sound state machine
  manageMainButtonAction() -> If pressed: execute action. If stopped: read inputs.
}
```

### Flow when timer is STOPPED

```
manageMainButtonAction()
  -> onStop()
     -> stopAllTimers()
     -> evaluateEncoderMod()    (reads mode encoder)
     -> evaluateEncoderPrec()   (reads precision encoder)
     -> evaluateButtons()       (reads encoder buttons)
     -> evaluateKey()           (reads keypad)
```

### Flow when timer is RUNNING

```
manageMainButtonAction()
  -> manageStarted()
     -> operationStatus.current.startedAction()
        -> focus() / countdown() / testStrip()
           -> startFocusTimer() / startCountdownTimer() / startTestStripTimer()
              -> appTimer.every(100, callback)
```

Timer callbacks are executed every 100ms by `appTimer.tick()`.

### Libraries used

| Library | Version | Purpose |
|---------|---------|---------|
| Encoder | 1.4.4 | Rotary encoder reading |
| Keypad | 3.1.1 | 4x4 matrix keypad |
| LiquidCrystal_I2C | 1.1.4 | I2C LCD (indirect dependency) |
| EncoderButton | 1.0.6 | Encoder button handling with debounce |
| ArduinoSTL | 1.3.3 | Minimal STL support for String |
| OneButton | 2.6.1 | Main button with click/hold detection |
| arduino-timer | 3.0.1 | Non-blocking timer with callbacks |
| WaveShare-Lib | local | Waveshare RGB LCD driver |

---

## Quick Command Reference

### Number keys (0-9)
Available only in COUNTDOWN and TEST STRIP. Sequential input up to 4 digits.

### Letter keys

| Key | Short press | Long press |
|-----|-------------|------------|
| `A` | Select channel A | - |
| `B` | Select channel B | - |
| `C` | Select channel C | - |
| `D` | Reset current channel | - |

### Special keys

| Key | In FOCUS | In COUNTDOWN / TEST STRIP | Long press |
|-----|----------|---------------------------|------------|
| `*` | Brightness -1 | Time -1 step (Seconds) / F-stop -1 (F-Stop) | Toggle Mute |
| `#` | Brightness +1 | Time +1 step (Seconds) / F-stop +1 (F-Stop) | Toggle Channel Relay |

### Encoders

| Encoder | Rotation | Button press |
|---------|----------|--------------|
| Mode (Left) | Change: FOCUS / COUNTDOWN / TEST STRIP | Toggle CO/PP (Test Strip only) |
| Precision (Right) | Change precision level (11 values) | Toggle type: Seconds (S) / F-Stop (F) |

### Main button

| State | Action |
|-------|--------|
| Timer stopped | Start current operation |
| Timer running | Stop current operation |

---

## Memory resources

| Resource | Usage | Available | Percentage |
|----------|-------|-----------|------------|
| RAM | 2050 bytes | 8192 bytes | 25.0% |
| Flash | 28082 bytes | 253952 bytes | 11.1% |
