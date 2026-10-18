# TECHNICAL — R080 Timer firmware rewrite

- **Feature:** `firmware-rewrite`
- **Date:** 2026-10-18
- **Source of decisions:** grilling session of 2026-10-18; `docs/firmware-rewrite/decisions.md`; `GLOSSARY.md`; requirements in `EARS.en.md`.
- **Code read:** the whole firmware at tag `pre-rewrite` (`src/main.cpp`, `include/*.h`, `include/functions/*.h`, `lib/WaveShare-Lib/*`, `platformio.ini`), the library sources in `.pio/libdeps/megaatmega2560/` (OneButton 2.6.1, Keypad 3.1.1, Encoder 1.4.4), `Tone.cpp`, `HardwareSerial.h` and `Wire.h` from the `framework-arduino-avr` 5.2.0 core, the bootloader `bootloaders/stk500v2/stk500boot.c`, the disassembly of the current ISR.

## Overview

**Current state.** The firmware is a single translation unit: `src/main.cpp` includes the `include/functions/*.h` headers in a fixed order; they define functions and read and write global variables from `include/variables.h`. The timing engine already runs on Timer1 at 1 kHz (`include/functions/timer_isr.h`), but the rest has structural defects verified in the code:
- `onStop()` (`include/functions/running_mgmt.h`) calls `stopAllTimers()` on every loop pass, which re-triggers `emitStopSound()` and rewrites `showTime(timerCurrentValue)`: while idle the stop beep plays in a loop and the 7-segment display goes back to 0000;
- `setOperationStatus()` (`include/functions/operations.h`) calls `resetTime()` and clears the time loaded by `loadSettings()`;
- the `lib/WaveShare-Lib` library runs `delay(5)` for every I²C byte in `Waveshare_LCD1602_RGB::send()`, and `evaluateEncoderPrec()` redraws the precision on every pass: the idle loop takes about 110 ms and short presses are lost;
- the ISR computes `g_remainingMs % 1000` on 32 bits and calls `__udivmodsi4` every millisecond;
- no logic can be tested without the board.

**Goal.** New firmware in three layers: a domain library in pure C++ (`lib/r080_core/`, NEW) tested on the PC; thin Arduino drivers (`src/drivers/`, NEW) that implement the domain interfaces; a rewritten `src/main.cpp` that wires them together. The code at tag `pre-rewrite` remains for reference only.

## Constraints and invariants

- **No blocking in the loop.** No `delay()` or busy waiting after startup; every driver works in short steps. The loop calls `wdt_reset()` on every pass.
- **Minimal ISR.** The Timer1 ISR performs only additions, comparisons and writes to `PORTL`, with no divisions and no calls to non-inline functions.
- **Shared variables.** Every read or write from the main code of state shared with an ISR, if wider than 8 bits, happens inside `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` (rule already in `CLAUDE.md`).
- **EEPROM.** Writes use only `eeprom_update_*`, never `eeprom_write_*` (rule already in `CLAUDE.md`).
- **RAM.** No `String`, no `ArduinoSTL`, no dynamic allocation (`new`/`malloc`); constant texts live in `PROGMEM`. On AVR the vtables of virtual interfaces take RAM: the number of interfaces stays small.
- **Units.** Internal time is in milliseconds `uint32_t`; the tenth of a second is the interface unit (REQ-11.5).
- **Domain without Arduino.** No file in `lib/r080_core/` includes `Arduino.h`, `avr/*` or Arduino libraries.
- **Superseded invariants.** The "single translation unit" model and the fixed include order described in `CLAUDE.md` no longer apply and must be rewritten (C-26).

## Changes

### C-1 — Repository: branches and tag
- **Satisfies:** REQ-12.4
- **Current state:** branches `feature/rewrite_to_improve_stability_and_performance` and `feature/micropython-esp32-port`; local tag `pre-rewrite` on `c36256f`, not pushed to the remote. `CLAUDE.md` (§2) and `docs/agents/issue-tracker.md` (table "Platform labels") mention the old names.
- **Change:** on `main`, update the names in `CLAUDE.md` and `docs/agents/issue-tracker.md`; create and push `feature/arduino-mega-firmware` and `feature/esp32-micropython-firmware` from the current branches, rebased on `main`; push the `pre-rewrite` tag; delete the old remote branches only after explicit confirmation from the user. The ESP32 branch's `CLAUDE.md` is updated on that branch.
- **Expected behaviour:** `git branch -r` shows the two new names; `git tag` shows `pre-rewrite` on the remote too.
- **Verification:** `git ls-remote --heads origin` and `git ls-remote --tags origin`.

### C-2 — `platformio.ini`: Mega and native environments
- **Satisfies:** REQ-10.3, REQ-11.3, REQ-11.5
- **Current state:** a single `[env:megaatmega2560]` environment with `lib_deps` `paulstoffregen/Encoder`, `chris--a/Keypad`, `marcoschwartz/LiquidCrystal_I2C`, `stutchbury/EncoderButton`, `mathertel/OneButton`.
- **Change:**
  ```ini
  [platformio]
  default_envs = megaatmega2560

  [env:megaatmega2560]
  platform = atmelavr
  board = megaatmega2560
  framework = arduino
  build_unflags = -std=gnu++11
  build_flags = -std=gnu++17 -Wall -Wextra -DFW_VERSION=\"2.0.0\"
  lib_deps =
    paulstoffregen/Encoder@^1.4.4
    chris--a/Keypad@^3.1.1
    mathertel/OneButton@^2.6.1

  [env:native]
  platform = native
  test_framework = unity
  build_flags = -std=gnu++17 -Wall -Wextra
  lib_ignore = Encoder, Keypad, OneButton
  ```
  `LiquidCrystal_I2C` and `EncoderButton` are removed.
- **Expected behaviour:** `pio run` builds for the Mega; `pio test -e native` builds `lib/r080_core` and the tests with the PC compiler.
- **Verification:** both commands finish with exit 0.

### C-3 — `.github/workflows/firmware.yml` (NEW): CI
- **Satisfies:** REQ-11.4
- **Current state:** no CI.
- **Change:** workflow on `push` and `pull_request`: installs Python and PlatformIO, runs `pio run -e megaatmega2560` and `pio test -e native`, with a cache of `~/.platformio`.
- **Expected behaviour:** every push shows the build and test status on GitHub.
- **Verification:** a push with a deliberately broken test makes the workflow fail.

### C-4 — Removal of the previous firmware
- **Satisfies:** REQ-10.3, REQ-11.1
- **Current state:** `include/types.h`, `include/constants.h`, `include/variables.h`, `include/functions/{actions,base,config,encoder_mgmt,fstop,keypad_mgmt,operations,presentation,running_mgmt,sound,storage,timer_isr,timer_manager}.h`, `lib/WaveShare-Lib/Waveshare_LCD1602_RGB.{h,cpp}`.
- **Change:** the listed files are deleted in the same commit that brings in the new `src/main.cpp` (C-24), so every commit builds. They remain available at tag `pre-rewrite`.
- **Expected behaviour:** no leftover references to `operationStatus`, `timeSet`, `appoTimeSet`, `g_remainingMs` and the like.
- **Verification:** `grep -rn "timeSet\|operationStatus\|Waveshare_LCD1602_RGB" src lib include` finds nothing; `pio run` ok.

### C-5 — `lib/r080_core/Hal.h`, `Time.h` (NEW): interfaces and units
- **Satisfies:** REQ-11.1, REQ-11.2, REQ-11.5
- **Current state:** the code calls `lcd`, `sevenSegmentsDisplay`, `tone()`, `eeprom_*`, `RELAY_ON()` directly.
- **Change:** abstract interfaces used by the domain:
  ```cpp
  struct Lcd       { virtual bool writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) = 0;
                     virtual bool setBrightness(uint8_t level) = 0; virtual bool online() const = 0; };
  struct SevenSeg  { virtual void showDeciseconds(uint16_t ds) = 0; virtual void showMinSec(uint16_t mmss) = 0; virtual void setBrightness(uint8_t level) = 0; };
  struct Buzzer    { virtual void play(uint16_t hz, uint16_t ms) = 0; virtual void stop() = 0; };
  struct Storage   { virtual void read(uint16_t addr, uint8_t* dst, uint16_t n) = 0;
                     virtual void update(uint16_t addr, const uint8_t* src, uint16_t n) = 0; virtual bool ready() = 0; };
  struct EventLog  { virtual void line(const char* text) = 0; };
  struct Clock     { virtual uint32_t nowMs() const = 0; };
  ```
  `Time.h`: `constexpr uint32_t MS_PER_DS = 100; constexpr uint16_t DS_MIN = 1, DS_MAX = 9999;` and the functions `dsToMs()`, `msToDs()`. The relay has no interface in the domain: it is driven only by `TimingCore` (C-6) through the output bits and by the driver (C-16).
- **Expected behaviour:** the domain builds on the PC without Arduino headers.
- **Verification:** `pio test -e native` builds; `grep -rn "Arduino.h\|avr/" lib/r080_core` finds nothing.

### C-6 — `lib/r080_core/TimingCore.{h,cpp}` (NEW): ISR logic
- **Satisfies:** REQ-1.1, REQ-1.2, REQ-1.3, REQ-1.5, REQ-1.6, REQ-3.3, REQ-3.4, REQ-3.11, REQ-3.12, REQ-3.14, REQ-8.3, REQ-11.3
- **Current state:** the logic lives inside `ISR(TIMER1_COMPA_vect)` in `include/functions/timer_isr.h`, with `g_modeActive` (0–3), `g_remainingMs`, `g_elapsedMs`, `g_nextStepTargetMs` and the 32-bit modulo for the metronome; step-by-step mode turns the relay off from the main code in `onTestStripStep()`.
- **Change:** pure class called once per tick by the ISR:
  ```cpp
  enum class Run : uint8_t { Idle, Focus, Countdown, StripContinuous, StripStepped };
  enum : uint8_t { EV_END = 1, EV_STEP = 2, EV_METRONOME = 4, EV_PREWARN = 8,
                   EV_FOCUS_WARN = 16, EV_FOCUS_OFF = 32, EV_FRAME = 64 };
  struct TickOut { uint8_t events; bool relayOff; };

  class TimingCore {
   public:
    void startFocus(uint32_t limitMs);
    void startCountdown(uint32_t durationMs);
    void startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs);
    void setNextStep(uint32_t nextStepMs);
    void cancel();
    void setCorrection(int16_t ppm);           // delegates to ClockCorrection (C-7)
    TickOut tick();                             // called by the ISR, 1 kHz
    uint32_t elapsedMs() const; uint32_t remainingMs() const; Run run() const;
   private:
    Run run_; uint32_t elapsed_, target_, limit_, next_; uint16_t msInSecond_; uint8_t frame_;
  };
  ```
  Rules of `tick()`: advances `elapsed_` by 0, 1 or 2 ms according to the Clock correction; `msInSecond_` (16 bits) counts up to 1000 and raises `EV_METRONOME` only in `Countdown`; `frame_` raises `EV_FRAME` every 100 ms for the display refresh; in `Countdown`, at `elapsed_ >= target_` → `relayOff`, `EV_END`, `Idle`; in Test strip, when `next_ - elapsed_` equals 300, 200, 100 ms → `EV_PREWARN`, at `elapsed_ >= next_` → `EV_STEP` and, if `StripStepped`, `relayOff` and pause; at 999.9 s → `relayOff`, `EV_END`; in `Focus`, at `limit_ - 10000` → `EV_FOCUS_WARN`, at `limit_` → `relayOff`, `EV_FOCUS_OFF`. The metronome counts from the start of the Exposure (REQ-1.6), not from the remaining time.
- **Expected behaviour:** all timing decisions are taken at the exact tick, without depending on the loop.
- **Verification:** native tests in `test/test_timing_core/` that call `tick()` N times and check events and `relayOff` at the exact ms (10.0 s countdown → `relayOff` at tick 10000; metronome at ticks 1000, 2000…; pre-warning at −300/−200/−100; Focus at limit −10 s and at the limit); disassembly of the ISR with no `call` (see C-16).

### C-7 — `lib/r080_core/ClockCorrection.{h,cpp}` (NEW): Clock correction
- **Satisfies:** REQ-1.4, REQ-1.7, REQ-9.12
- **Current state:** does not exist.
- **Change:** parts-per-million accumulator run on every tick:
  ```cpp
  // returns how many ms to count in this tick: 0, 1 or 2
  inline uint8_t ClockCorrection::step() {
    acc_ += ppm_;                                   // int32_t acc_, int16_t ppm_ in [-1000, 1000]
    if (acc_ >=  1000000L) { acc_ -= 1000000L; return 2; }
    if (acc_ <= -1000000L) { acc_ += 1000000L; return 0; }
    return 1;
  }
  ```
  `ppmFromMeasurement(uint32_t nominalMs, uint32_t measuredMs, int16_t currentPpm)` computes the new correction: `(1 + cur/1e6) · measured/nominal − 1`, in ppm, clamped to ±1000.
- **Expected behaviour:** with +200 ppm the counted time advances by one extra ms every 5000 ticks.
- **Verification:** native tests over 10⁶ ticks with ±1000, ±200, 0 ppm; test of the calculation from a measurement. The real accuracy is measured with `tools/calibrate.py` (C-29).

### C-8 — `lib/r080_core/Precision.{h,cpp}` (NEW): tables and F-Stop
- **Satisfies:** REQ-4.1, REQ-4.2, REQ-4.3, REQ-4.5, REQ-3.10
- **Current state:** `FSTOP_PRECISIONS[]` and `SECONDS_PRECISIONS[]` in `include/types.h` (PROGMEM strings `Pr:1/3  `…); `FStopState::getCalculatedTime()` in `include/functions/fstop.h` uses `pow(2.0, stopOffset/precisionValue)` with a 1..9999 clamp.
- **Change:** tables `kFStopDenominators[11] = {1,2,3,4,6,8,12,16,24,32,64}` and `kSecondsSteps[11] = {1,2,3,4,5,6,7,8,10,15,20}`; pure functions `uint16_t fstopTime(uint16_t baseDs, int8_t offset, uint8_t denom)` (rounded to the tenth, clamped to `DS_MIN..DS_MAX`) and `uint16_t secondsStep(uint16_t ds, int8_t dir, uint8_t secs)`. It uses `exp2f`/`powf` in float: the formula stays the current one.
- **Expected behaviour:** the same values as today for base, offset and precision.
- **Verification:** native tests with a table of cases (base 10.0 s, 1/3, offset +1 → 12.6 s; offset +2 → 15.9 s; clamp at the limits).

### C-9 — `lib/r080_core/TestStripPlan.{h,cpp}` (NEW): test strip steps
- **Satisfies:** REQ-3.9, REQ-3.10, REQ-3.12, REQ-3.13, REQ-3.14, REQ-7.6, REQ-7.8
- **Current state:** `onTestStripStep()` (`include/functions/timer_manager.h`) computes the next target from the rounded `elapsedMs/100` (`timerCurrentDecis * 2^(1/p)`), accumulating the rounding error; after a manual stop `g_elapsedMs` does not go back to zero.
- **Change:** `struct TestStripPlan { uint16_t baseDs; PrecisionKind kind; uint8_t value; uint32_t stepMs(uint8_t k) const; }` with `stepMs(0) = T`, seconds `T + k·P`, F-Stop `T·2^(k/p)` always computed from the base; it guarantees strictly increasing steps of at least 0.1 s (with 1/64 and a small base, rounding would produce equal steps); limit 999.9 s. Every start after a Cancellation or a completion restarts from `k = 0`.
- **Expected behaviour:** sequences 10.0 → 14.1 → 20.0 → 28.3 s (F-Stop 1/2) without drift.
- **Verification:** native tests on seconds and F-Stop sequences, minimum base, 999.9 s limit, restart.

### C-10 — `lib/r080_core/Settings.{h,cpp}`, `Crc8.h` (NEW): Channels and EEPROM format
- **Satisfies:** REQ-2.5, REQ-6.1, REQ-6.4, REQ-6.5, REQ-6.6, REQ-6.7, REQ-6.8, REQ-7.2, REQ-7.12, REQ-9.3, REQ-9.7, REQ-9.8, REQ-9.11
- **Current state:** `include/functions/storage.h` uses magic `0xA5` at address 0 and 13 bytes (brightness, mute, channel, 3 times, 3 relay enables), with no CRC or version; mode and precision are not saved.
- **Change:** new layout with two independent blocks, each with its own CRC8 (polynomial 0x07):
  ```cpp
  struct ChannelRecord { uint8_t mode, precisionKind, precisionIndex, advance;
                         uint16_t timeDs, baseDs; int8_t offset; uint8_t relayEnabled; };   // 10 B
  struct GlobalRecord  { uint8_t activeChannel, mute, lcdLevel, segLevel,
                         lcdExposureLevel, focusLimitIndex; int16_t clockPpm; };              // 8 B
  // Settings block @0x000: 'R','8', version=1, length | GlobalRecord | ChannelRecord[3] | crc8  = 43 B
  // Counters block    @0x040: 'R','C', version=1, length | powerOns, watchdogResets (u16)
  //                             | exposures (u32) | lampSeconds (u32) | crc8                        = 17 B
  ```
  `SettingsCodec::load()` returns the factory values if magic, version, length or CRC do not match (also with the old `0xA5` format); `save()` starts a step-by-step save: on every loop pass, if `Storage::ready()`, `Settings::service()` writes a single byte that differs from the EEPROM, the CRC last, so the loop never stalls for the ~3.4 ms of each write and an interrupted save leaves a wrong CRC (factory values at the next boot). The same holds for the counters (`Counters::service()`), except at startup, in `setup()`, where waiting is allowed. Changes mark the settings as "to be saved" and `Settings::service(nowMs)` saves after 2 s without new changes, or immediately when an Exposure starts. The same delay applies to all changes, not only to digits. Factory values: Channel A, all Channels in Countdown, Seconds 1 s, continuous, 10.0 s, relay enabled; mute off; Focus 5 minutes; correction 0 ppm; 7-segment brightness level 3 (value 10), LCD level 3, LCD during exposure level 1 (REQ-7.12).
- **Expected behaviour:** after a restart the saved state is restored; a corrupted or old EEPROM leads to the factory values.
- **Verification:** native tests with `FakeStorage`: round trip, wrong CRC, wrong version, `0xA5` format, 2 s delayed save, no write if nothing changed.

### C-11 — `lib/r080_core/KeyEntry.{h,cpp}` (NEW): calculator-style entry
- **Satisfies:** REQ-5.1, REQ-5.2, REQ-5.3
- **Current state:** in `evaluateKey()` (`include/functions/keypad_mgmt.h`) `nCifra` counts up to 3 and then keeps multiplying by 10 (16-bit `int` overflow at the sixth digit); it is not reset after an Exposure.
- **Change:** `class KeyEntry { uint16_t push(uint8_t digit); void breakEntry(); }`: `push` returns `(prev % 1000) * 10 + digit` if an entry is in progress, otherwise `digit`; `breakEntry()` is called by the Controller (C-12) on every other action.
- **Expected behaviour:** 1, 2, 5 → 0.1 / 1.2 / 12.5 s; 1, 2, 3, 4, 5 → 234.5 s; after an Exposure the first digit starts a new time.
- **Verification:** native tests.

### C-12 — `lib/r080_core/Controller.{h,cpp}` (NEW): user interface state machine
- **Satisfies:** REQ-2.4, REQ-3.1, REQ-3.2, REQ-3.5, REQ-3.6, REQ-3.7, REQ-3.8, REQ-3.12, REQ-3.13, REQ-3.15, REQ-4.2, REQ-4.3, REQ-4.5, REQ-4.6, REQ-4.7, REQ-5.4, REQ-5.5, REQ-5.6, REQ-5.7, REQ-5.8, REQ-5.9, REQ-5.10, REQ-6.2, REQ-6.3, REQ-8.1, REQ-3.16
- **Current state:** the logic is scattered across `manageMainButtonAction()`, `onStop()`, `evaluateKey()`, `evaluateEncoderMod()`, `evaluateEncoderPrec()`, `onModeButtonEvent()`, `onPrecisionButtonEvent()`, the `Operation` class with function pointers (`showPresentation`, `switchModality`, `switchPrecisionType`, `startedAction`) and the `isMainBtnPressed`/`isActionStarted` flags.
- **Change:** a single state machine that receives events and produces commands:
  ```cpp
  enum class Input : uint8_t { Digit, Star, Hash, StarHold, HashHold, ZeroStar, ZeroHash,
                               ChannelA, ChannelB, ChannelC, KeyD, ModeTurn, PrecTurn,
                               ModeClick, PrecClick, MainPress };
  enum class State : uint8_t { Idle, Running, StripPaused, Service };
  class Controller {
   public:
    void onInput(Input in, int8_t arg);          // arg: digit or direction of rotation
    void onTiming(uint8_t events);                // events from TimingCore
    // injected dependencies: TimingPort (atomic commands to TimingCore), Sound, Screen, Settings, KeyEntry
  };
  ```
  Main rules: in `Running` only `MainPress` is accepted (REQ-5.9); `MainPress` in `Idle` with time 0 in Countdown/Test strip → error beep; `MainPress` in `Running` → Cancellation; in `StripPaused` → next increment; `KeyD` clears time, base and offset; `StarHold`/`HashHold` → mute or Relay enable with a confirmation beep (for mute only when it is turned off); `ZeroStar`/`ZeroHash` and `Star`/`Hash` in Focus → 7-segment and LCD brightness; `ModeTurn` changes mode without touching the time; in Focus `Digit`, `PrecTurn` and `PrecClick` are ignored (REQ-4.6, REQ-5.1); `A/B/C` load the whole `ChannelRecord`. In `StripPaused`, `KeyD`, `A/B/C` and `ModeTurn` end the test strip: `TimingCore::cancel()`, plan reset to `k = 0`, 7-segment display back to the set time; then `A/B/C` switch Channel and `ModeTurn` changes mode, `KeyD` clears nothing (REQ-3.16).
- **Expected behaviour:** all user interface rules of EARS REQ-3, REQ-4, REQ-5 and REQ-6 are observable with fake drivers.
- **Verification:** native tests scenario by scenario through the whole-domain seam (C-25), with the real `TimingCore` and fake drivers.

### C-13 — `lib/r080_core/Screen.{h,cpp}` (NEW): display model and rendering
- **Satisfies:** REQ-2.6, REQ-2.7, REQ-4.4, REQ-7.1, REQ-7.3, REQ-7.4, REQ-7.5, REQ-7.6, REQ-7.7, REQ-7.8, REQ-7.9, REQ-7.10, REQ-3.7
- **Current state:** `include/functions/presentation.h` writes directly to the LCD on every call (`showPrecision()`, `showModalityType()`…), `showFstopOffset()` writes at column 5 overwriting the denominator; `showTime()` writes to the 7-segment display on every pass while idle.
- **Change:** `Screen` builds the desired image (2×16 characters, 7-segment value, brightness) from the Controller state, compares it with the last one sent and sends only the differences, at most one row per loop pass. Row 2 layout: columns 0–3 precision without prefix (` 5s`, `20s`, `1/3`, `1/12`) or `St:n` during the test strip, columns 4–6 F-Stop offset (`+02`), columns 9–12 Channel (`Ch:A`), column 14 mute note, column 15 relay icon. In Focus columns 0–8 stay blank. During the Exposure it asks the LCD for the exposure brightness. If `Lcd::online()` is false it keeps updating only the 7-segment display. The 7-segment display is updated on `EV_FRAME` (10 Hz) during the Exposure and only when the value changes while idle. The splash screen (`R080 Timer` + `FW_VERSION`, 1.5 s) is a timed state, not a `delay()`.
- **Expected behaviour:** the offset stays visible next to the precision; while idle the I²C bus and the serial line stay quiet if nothing changes.
- **Verification:** native tests on `FakeLcd` (expected content cell by cell, no writes without changes, offset at columns 4–6).

### C-14 — `lib/r080_core/SoundScheduler.{h,cpp}` (NEW): sounds
- **Satisfies:** REQ-1.5, REQ-1.6, REQ-3.4, REQ-8.1, REQ-8.2, REQ-8.3, REQ-8.4, REQ-8.5, REQ-8.6, REQ-8.7
- **Current state:** `include/functions/sound.h`: direct `emitSound()` and a `SoundPlayer` with only `STOP_SEQUENCE` (300 Hz 75/150/75/150/150 ms), based on `readTickMs()`; tones `TONE_UP = 600`, `TONE_DOWN = 300`.
- **Change:** constant sequences (`Start`, `Metronome`, `Step`, `PreWarn`, `End`, `Error`, `Confirm`, `FocusWarn`) as arrays of `{hz, ms}`; priority `End` > `Error` > `Step` > `PreWarn` > `Metronome` > `Confirm`; with mute on only `End` plays. `tick(nowMs)` advances the active sequence through `Buzzer`. Values in a single `kSounds` table (REQ-8.7):
  ```cpp
  // {hz, ms}; hz = 0 is a pause
  Start     = {{1000, 60}};
  Metronome = {{300, 80}};
  Step      = {{600, 150}};
  PreWarn   = {{800, 30}};                                        // played at -300, -200, -100 ms before the Step
  End       = {{600, 150}, {0, 80}, {450, 150}, {0, 80}, {300, 150}};
  Error     = {{150, 300}};
  Confirm   = {{1200, 40}};
  FocusWarn = {{450, 200}, {0, 100}, {450, 200}};
  ```
- **Expected behaviour:** every timing event produces a recognisable sound; with mute on only the End sound remains.
- **Verification:** native tests on `FakeBuzzer` (sequences, priority, mute); listening test in the Acceptance checklist; audio recording for REQ-1.6.

### C-15 — `lib/r080_core/Diagnostics.{h,cpp}`, `ServiceMenu.{h,cpp}` (NEW): service menu
- **Satisfies:** REQ-9.1, REQ-9.2, REQ-9.3, REQ-9.4, REQ-9.5, REQ-9.6, REQ-9.7, REQ-9.8, REQ-9.9, REQ-9.10, REQ-9.11, REQ-9.12, REQ-9.14, REQ-3.3
- **Current state:** does not exist.
- **Change:** `Diagnostics` keeps the maximum loop and ISR durations and the lost ticks (fed by drivers C-16 and C-24) and updates the counters in the counters block (C-10) at the end of an Exposure: `exposures` +1 for every Countdown and for every whole test strip, `lampSeconds` adding up the time with the relay on in any mode, Focus included (REQ-9.11); `powerOns` at every startup and `watchdogResets` as well after a watchdog reset. `ServiceMenu` is a Controller state (`State::Service`) with the entries Info, Counters, Health, Hardware test (relay, buzzer, 7-segment, LCD, keypad, encoders, main button), Calibration (calibration log, direct entry in ppm, entry of the measured duration of a 600.0 s Countdown with hundredths of a second, converted by `ppmFromMeasurement()`), LCD brightness during exposure, Focus maximum time (3/5/10/15/20 min), Factory reset (with a confirmation screen, `D` cancels), Exit (back to normal use without rebooting). Navigation (REQ-9.14): mode encoder to scroll, its button to enter or confirm, `D` to go back, digits for values. `EventLog` formats lines such as `t=12345 START mode=CD ch=A ms=10000` and `t=22346 END ms=10000 measured=10000`.
- **Expected behaviour:** the Service menu opens only by holding `D` at power-on; each test acts only on the chosen component.
- **Verification:** native tests of navigation and calculations; Acceptance checklist for the hardware tests.

### C-16 — `src/drivers/Timer1Clock.{h,cpp}` (NEW): Timer1, ISR and relay
- **Satisfies:** REQ-1.1, REQ-1.2, REQ-1.3, REQ-2.1, REQ-9.4
- **Current state:** `setupTimer1()` and `ISR(TIMER1_COMPA_vect)` in `include/functions/timer_isr.h`; `RELAY_ON()`/`RELAY_OFF()` macros on `PORTL` bit `PL2`; `relayStart()` in `include/functions/base.h`.
- **Change:** same register configuration (`TCCR1A = 0`, `TCCR1B = WGM12 | CS11 | CS10`, `OCR1A = 249`, `TIMSK1 |= OCIE1A`). The ISR calls `core.tick()` (C-6), applies `RELAY_OFF()` if `relayOff`, accumulates events in a `volatile uint8_t pendingEvents` and, on exit, reads `TCNT1` for the maximum ISR duration. Commands from the main code (`start*`, `setNextStep`, `cancel`) and `takeEvents()` are inside `ATOMIC_BLOCK`; `start*` and `cancel` clear, in the same block, the events not yet collected, which belong to the previous Exposure. Start turns the relay on with `RELAY_ON()` only if the Channel's Relay enable is active, after arming the core. Lost ticks: Timer4 free-running with prescaler 64 acts as an independent hardware reference and the main code compares the two counts every second. In `setup()` `PL2` is set as an output at low level as the very first instruction.
- **Expected behaviour:** relay turned off by the ISR at the exact ms in Countdown, step-by-step and Focus Auto-off.
- **Verification:** `avr-objdump -d` of the ISR with no `call`; oscilloscope or logic analyser on D47 (end of Countdown within 1 ms); ISR duration measurement from the Service menu.

### C-17 — `src/drivers/LcdWaveshare.{h,cpp}` (NEW): non-blocking LCD driver
- **Satisfies:** REQ-2.6, REQ-2.7, REQ-7.1, REQ-7.2, REQ-7.3, REQ-7.11, REQ-10.1
- **Current state:** `Waveshare_LCD1602_RGB::send()` runs `delay(5)` per byte; `setCursor()` + `write_char()` = 4 I²C transactions per character; `init()` uses `delay(50)`/`delay(5)`; addresses `LCD_ADDRESS 0x3E`, `RGB_ADDRESS 0x60` (PCA9633 controller, registers `REG_RED 0x04`, `REG_GREEN 0x03`, `REG_BLUE 0x02`, `REG_MODE1`, `REG_MODE2`, `REG_OUTPUT`).
- **Change:** `Wire.setClock(100000)` and `Wire.setWireTimeout(3000, true)`; `writeAt()` sends a single transaction for a run of contiguous characters (control byte `0x80` + DDRAM address, then `0x40` + characters); no `delay` after initialisation, which uses a small state machine with the timings required by the controller; colour always `setReg(REG_RED, kLcdLevels[level - 1])` with `kLcdLevels[8] = {4, 8, 16, 28, 48, 80, 140, 255}`, `REG_GREEN = REG_BLUE = 0`, `REG_MODE2` without blink (`0x00`); `online()` becomes false at the first `endTransmission()` error or timeout, with a new attempt every 2 s recorded in the log; custom characters for the note and the plug loaded at initialisation.
- **Expected behaviour:** a full row costs one transaction of about 2 ms; a disconnected LCD does not block the loop.
- **Verification:** measurement of the maximum loop duration from the Service menu; test with the LCD disconnected during a Countdown (Acceptance checklist).

### C-18 — `src/drivers/SevenSegSerial3.{h,cpp}` (NEW): 7-segment display on Serial3
- **Satisfies:** REQ-10.2, REQ-10.4, REQ-7.2, REQ-7.4, REQ-7.7, REQ-7.11
- **Current state:** `SoftwareSerial sevenSegmentsDisplay(1, DISPLAY_7_SEG_PIN)` on A2 at 9600 baud (with RX pin 1 shared with `Serial`); commands `0x76` (reset), `0x77` + `0b00000100` (decimal point), `0x7A` + brightness; `showTime()` sends `"%04i"`.
- **Change:** `Serial3.begin(9600)` on pin 14 (TX3); same display commands; brightness `0x7A` + `kSegLevels[level - 1]` with `kSegLevels[8] = {2, 5, 10, 20, 40, 80, 150, 255}` (level 3 reproduces today's value 10); the 64-byte hardware transmit buffer makes writes non-blocking; the driver writes only when the value changes. Wiring: the DIN wire moves from A2 to D14.
- **Expected behaviour:** interrupts are never disabled while the 7-segment display is updated.
- **Verification:** Metronome jitter with an audio recording (REQ-1.6); maximum ISR duration unchanged during the Exposure.

### C-19 — `src/drivers/BuzzerTone.{h,cpp}` (NEW): buzzer
- **Satisfies:** REQ-8.1, REQ-8.4, REQ-8.6
- **Current state:** `tone(BUZZER_PIN, …)` and `noTone()` on D49 (Timer2) in `include/functions/sound.h`.
- **Change:** implementation of `Buzzer` with `tone()`/`noTone()` on the same pin. `tone()` uses Timer2 and writes to `PORTL` bit `PL0`, the same register as the relay: relay writes from the main code are inside `ATOMIC_BLOCK` so the buzzer bit is not lost.
- **Expected behaviour:** the same sounds as today where they do not change.
- **Verification:** Acceptance checklist (listening to all sequences from the Service menu).

### C-20 — `src/drivers/InputDevices.{h,cpp}` (NEW): keypad, encoders and buttons
- **Satisfies:** REQ-2.4, REQ-5.5, REQ-5.6, REQ-5.8, REQ-5.10, REQ-5.11, REQ-5.12
- **Current state:** `Keypad` with `setDebounceTime(50)` and the default 500 ms hold (`Keypad.cpp`, `setHoldTime(500)`); after a HOLD a `RELEASED` still arrives and runs the normal action; `Encoder` read as an absolute position (`read() >> 2` modulo 3 or 11); `EncoderButton` for the encoder buttons; `OneButton` with `attachClick`, `setClickMs(100)`.
- **Change:** `Keypad` with `setHoldTime(1000)`; for each key a "hold fired" flag that suppresses the event on release; combination: `*`/`#` released while `0` is pressed produce `ZeroStar`/`ZeroHash` and suppress the release of `0`. Encoders: read the difference from the previous reading, divided by 4 (one detent), and generate `ModeTurn`/`PrecTurn` with the direction. Buttons: three `OneButton` (pins 2, 17, 16) with `setDebounceMs(20)`; the main one uses `attachPress()`, which fires on entering the pressed state after debouncing, the others `attachClick()`. All inputs are read on every loop pass, which stays short thanks to C-13 and C-17.
- **Expected behaviour:** start on press, no double action after a HOLD, no lost presses.
- **Verification:** Service menu hardware tests; Acceptance checklist (repeated short presses, HOLD, combinations).

### C-21 — `src/drivers/EepromStorage.{h,cpp}` (NEW): EEPROM
- **Satisfies:** REQ-6.5, REQ-6.8
- **Current state:** `eeprom_read_byte`/`eeprom_update_byte`/`eeprom_update_word` at fixed addresses in `include/functions/storage.h`.
- **Change:** implementation of `Storage` with `eeprom_read_block()`, `eeprom_update_block()` and `eeprom_is_ready()`.
- **Expected behaviour:** writes only the changed bytes.
- **Verification:** write counter in a debug build; Acceptance checklist (power off and on).

### C-22 — `src/drivers/SystemGuard.{h,cpp}` (NEW): watchdog, brown-out and reset cause
- **Satisfies:** REQ-2.1, REQ-2.2, REQ-2.3, REQ-2.8, REQ-9.9
- **Current state:** no watchdog. The `stk500v2` bootloader reads `MCUSR`, clears it and, if the reset comes from the watchdog, jumps straight to the application: the firmware cannot read the reset cause from `MCUSR`.
- **Change:** watchdog in interrupt + reset mode with a 250 ms timeout (`WDTCSR = WDIE | WDE | WDP2`): at the first timeout `ISR(WDT_vect)` runs `RELAY_OFF()` and writes a marker into an `__attribute__((section(".noinit")))` variable; at the second timeout (500 ms in total) the board resets. At startup the marker distinguishes a watchdog reset from a normal power-on and is then cleared. For brown-out the factory fuse threshold (2.7 V) stays: a reset caused by a voltage drop looks like a normal power-on. The 4.3 V threshold via ISP is in the backlog. No Exposure is resumed after a reset: the initial state is always `Idle` with the relay off.
- **Expected behaviour:** a stuck loop turns the relay off within 250 ms and resets within 500 ms; after the reset `RESET: WDT` is shown for 3 s.
- **Verification:** test build with an infinite loop triggered from the Service menu; logic analyser on D47.

### C-23 — `src/drivers/UsbLog.{h,cpp}` (NEW): USB log
- **Satisfies:** REQ-9.10, REQ-9.6
- **Current state:** `Serial.begin(9600)` and only the `=== R080 TIMER PRONTO ===` banner.
- **Change:** `Serial.begin(115200)`; implementation of `EventLog` that writes only if there is room in `Serial.availableForWrite()`, so it never blocks the loop (lines that do not fit are counted as lost). During the calibration log it writes one `CAL ms=<count>` line per second, aligned to the Timer1 tick.
- **Expected behaviour:** `pio device monitor -b 115200` shows startup, Exposures with measured duration, and errors.
- **Verification:** Acceptance checklist.

### C-24 — `src/main.cpp`: composition and loop
- **Satisfies:** REQ-11.2, REQ-2.1, REQ-7.9, REQ-9.1, REQ-9.4
- **Current state:** `setup()` with `delay(3000)`, `loadSettings()` followed by `initializeOperations()` (which clears the time); `loop()` with an atomic read of the `g_*` flags and `manageMainButtonAction()`.
- **Change:** `setup()`: relay as output at low level, `SystemGuard`, reading of the `D` key for the Service menu, settings load, driver initialisation, start of Timer1 and Timer4. `loop()`: `wdt_reset()`, `takeEvents()`, input reading, `Controller`, `SoundScheduler::tick()`, `Screen::render()`, `Settings::service()`, loop duration measurement for `Diagnostics`. Pins in `src/drivers/Pins.h` (NEW), with the same values as `include/constants.h` except the 7-segment display.
- **Expected behaviour:** no `delay()` after startup; a short loop in every state.
- **Verification:** maximum loop duration from the Service menu; full Acceptance checklist.

### C-25 — `test/` (NEW): native tests
- **Satisfies:** REQ-11.3
- **Current state:** PlatformIO's `test/README`, no tests.
- **Change:** two test seams, written before the code (TDD). **Seam 1, whole domain** (`test_timer`): the `Controller` with the real `TimingCore`, settings, `Screen` and `SoundScheduler`, wired to fake drivers in `test/fakes/` (`FakeLcd`, `FakeSevenSeg`, `FakeBuzzer`, `FakeStorage`, `FakeLog`) and to `SimClock`, which holds the real `TimingCore`, keeps the simulated relay state and advances time tick by tick when the test asks. Tests act only through user input and elapsed time and check only observable effects (relay, LCD, 7-segment, buzzer, EEPROM bytes, log). **Seam 2, `TimingCore` alone** (`test_timing_core`): millisecond edges, Clock correction, Pre-warning, Auto-off, the 999.9 s limit. No fakes of internal modules (no `FakeTimingPort`). Pure calculations (F-Stop, CRC8, test strip steps) are verified through these two seams; a direct test is allowed only for an edge case unreachable from outside.
- **Expected behaviour:** `pio test -e native` covers all the domain logic.
- **Verification:** local run and in CI (C-3).

### C-26 — `CLAUDE.md` and agent documents
- **Satisfies:** REQ-11.1, REQ-12.4
- **Current state:** `CLAUDE.md` §3 describes "Single Translation Unit", the include order with `timer_isr.h` before `base.h`, `channel` 1..3.
- **Change:** new invariants: three layers, domain without Arduino, no blocking in the loop, ISR without divisions, `ATOMIC_BLOCK`, `eeprom_update_*`, no dynamic allocation, `pio test -e native` commands; new branch names (see C-1).
- **Expected behaviour:** an agent reading `CLAUDE.md` finds rules consistent with the new code.
- **Verification:** review.

### C-27 — User and technical documentation
- **Satisfies:** REQ-12.1, REQ-12.3, REQ-12.5, REQ-10.5
- **Current state:** `README.md`, `README-IT.md` (they still mention `ArduinoSTL`), `DOCS.md`, `DOCS-IT.md`, `TECNICO.md`, `DIAGRAMS.md`, `PINOUT.md` and the PDFs `TECNICO.pdf`, `DIAGRAMS.pdf`, `PINOUT.pdf`.
- **Change:** user manual and README rewritten in Italian and English; `PINOUT.md` updated (7-segment display on D14, 10 kΩ pull-down on D47); the manual describes calibration with `tools/calibrate.py` and, as an alternative without a PC, with the audio recording of the beeps of a 600.0 s Countdown; `TECNICO.md` and `DIAGRAMS.md` replaced by new technical documentation; PDFs removed; `CHANGELOG.md` (NEW) starting from `v2.0.0`; `docs/refactoring-timing/` left untouched.
- **Expected behaviour:** documentation consistent with the released firmware.
- **Verification:** review before release.

### C-28 — `docs/firmware-rewrite/checklist-collaudo.md` (NEW): release
- **Satisfies:** REQ-12.1, REQ-12.2, REQ-1.4, REQ-1.6
- **Current state:** per-phase checklists in `docs/refactoring-timing/verify/`, never filled in.
- **Change:** a single Acceptance checklist with the on-board tests for every requirement not covered by the native tests (time measurements, Metronome jitter, lights, sounds, LCD faults, watchdog, persistence), with space for results and signature; the release is the `vX.Y.Z` tag on the tested commit.
- **Expected behaviour:** every released version has a signed checklist.
- **Verification:** a signed checklist exists for the `v2.0.0` tag.

### C-29 — `tools/calibrate.py` (NEW): calibration from the PC
- **Satisfies:** REQ-9.13, REQ-1.4
- **Current state:** does not exist.
- **Change:** Python script (standard library plus `pyserial` only) that opens the USB port at 115200 baud, reads the `CAL ms=<count>` lines for 30 minutes, records for each one the PC clock instant (`time.monotonic_ns()`), estimates the slope with a linear regression (which absorbs the USB jitter) and prints the Clock correction in ppm to enter in the Service menu.
- **Expected behaviour:** with the Mega's crystal it prints a value typically within ±50 ppm, with an uncertainty of a few ppm.
- **Verification:** test of the script with a synthetic log with a known drift; real test on the board (Acceptance checklist).

### C-30 — Wiring: pull-down on the relay
- **Satisfies:** REQ-10.5, REQ-2.1
- **Current state:** D47 connected directly to the relay module input; the module may already have its own pull-down resistor.
- **Change:** 10 kΩ resistor between D47 and GND. The module is active high (the firmware switches it on with `RELAY_ON()` = high level), so the resistor is harmless even in parallel with an existing one.
- **Expected behaviour:** relay off from power-on until the first firmware command.
- **Verification:** logic analyser or LED on D47 during 10 consecutive power-ons (Acceptance checklist).

## Implementation order

1. **C-1** branches and tag.
2. **C-2** `platformio.ini` with the native environment; **C-3** CI. The old firmware keeps building.
3. Domain with tests written before the code (**C-25** grows with each module): **C-5** → **C-8** → **C-7** → **C-6** → **C-9** → **C-10** → **C-11** → **C-14** → **C-13** → **C-12** → **C-15**. The library is not used yet by the old firmware, which keeps building.
4. Drivers: **C-16**, **C-17**, **C-18**, **C-19**, **C-20**, **C-21**, **C-22**, **C-23**, each built with a small test `main` in the Mega environment; in parallel **C-30** (wiring) and **C-29** (calibration script).
5. **C-24** new `src/main.cpp` together with **C-4** removal of the old code, in the same commit.
6. **C-26** `CLAUDE.md`; **C-27** documentation; **C-28** checklist and `v2.0.0` release.

## Technical risks

| Risk | Impact | Mitigation |
|---|---|---|
| The bootloader clears `MCUSR` and the factory brown-out threshold is 2.7 V, below the 4.5 V required at 16 MHz | Unpredictable behaviour between 4.5 and 2.7 V; voltage drop not recognised | `.noinit` marker for the watchdog; 4.3 V threshold via ISP in the backlog |
| Pin 47 in high impedance during the bootloader | Lamp on for an instant at power-on | 10 kΩ pull-down (C-30) |
| USB jitter and PC latency during calibration | Error in the correction | Linear regression over 30 minutes of samples (C-29) |
| Interface vtables in RAM on AVR | A few tens of bytes | Few interfaces; measurement with `pio run` (17.9% of RAM today) |
| `float`/`powf` on the Mega for F-Stop | Flash and computation time | Computation only in the loop, never in the ISR; results verified in the native tests |
| EEPROM wear of the counters (least significant byte written at every Exposure, ~100 000 cycles) | Unreliable counters after ~100 000 Exposures | Acceptable for v1; ring of slots if needed |
| `PORTL` shared by relay (`PL2`) and buzzer (`PL0`, written by `tone()` in an ISR) | Loss of a buzzer edge | Relay writes from the main code inside `ATOMIC_BLOCK` (C-19) |
| Surprise in avr-gcc 7.3 compatibility with `-std=gnu++17` | Build failure | Check in the first commit of C-2; fall back to `gnu++14` |
| Wire with the timeout enabled resets the I²C bus mid-transaction | Garbled characters on the LCD | `Screen` redraws the whole image after every error |

## Traceability matrix

| REQ | Changes | Verification |
|---|---|---|
| REQ-1.1 | C-6, C-16 | Native tests; register inspection |
| REQ-1.2 | C-6, C-16 | Native tests; logic analyser on D47 |
| REQ-1.3 | C-6, C-16 | Native tests; logic analyser on D47 |
| REQ-1.4 | C-7, C-28, C-29 | External measurement over 60.0 s after calibration |
| REQ-1.5 | C-6, C-14 | Native tests; audio recording |
| REQ-1.6 | C-6, C-14, C-28 | Native tests; audio recording, σ < 1 ms |
| REQ-1.7 | C-7 | Native tests |
| REQ-2.1 | C-16, C-22, C-24, C-30 | Logic analyser at power-on |
| REQ-2.2 | C-22 | Test build with a stuck loop |
| REQ-2.3 | C-22 | Adjustable power supply below 2.7 V |
| REQ-2.4 | C-12, C-20 | Acceptance checklist (bounces) |
| REQ-2.5 | C-10 | Native tests |
| REQ-2.6 | C-13, C-17 | Acceptance checklist: LCD disconnected during the Countdown |
| REQ-2.7 | C-13, C-17 | Maximum loop duration with the LCD disconnected |
| REQ-2.8 | C-22 | Acceptance checklist: cut the power during an Exposure |
| REQ-3.1 | C-12 | Native tests |
| REQ-3.2 | C-12 | Native tests; Acceptance checklist |
| REQ-3.3 | C-6, C-15 | Native tests |
| REQ-3.4 | C-6, C-14 | Native tests |
| REQ-3.5 | C-12 | Native tests |
| REQ-3.6 | C-12 | Native tests |
| REQ-3.7 | C-12, C-13 | Native tests |
| REQ-3.8 | C-12 | Native tests |
| REQ-3.9 | C-9 | Native tests |
| REQ-3.10 | C-8, C-9 | Native tests |
| REQ-3.11 | C-6 | Native tests |
| REQ-3.12 | C-6, C-9, C-12 | Native tests |
| REQ-3.13 | C-9, C-12 | Native tests |
| REQ-3.14 | C-6, C-9 | Native tests |
| REQ-3.15 | C-12 | Native tests |
| REQ-3.16 | C-12 | Native tests |
| REQ-4.1 | C-8 | Native tests |
| REQ-4.2 | C-8, C-12 | Native tests |
| REQ-4.3 | C-8, C-12 | Native tests |
| REQ-4.4 | C-13 | Native tests on `FakeLcd` |
| REQ-4.5 | C-8, C-12 | Native tests |
| REQ-4.6 | C-12 | Native tests |
| REQ-4.7 | C-12 | Native tests |
| REQ-5.1 | C-11 | Native tests |
| REQ-5.2 | C-11 | Native tests |
| REQ-5.3 | C-11 | Native tests |
| REQ-5.4 | C-12 | Native tests |
| REQ-5.5 | C-12, C-20 | Native tests; Acceptance checklist |
| REQ-5.6 | C-12, C-20 | Native tests; Acceptance checklist |
| REQ-5.7 | C-12 | Native tests |
| REQ-5.8 | C-12, C-20 | Native tests; Acceptance checklist |
| REQ-5.9 | C-12 | Native tests |
| REQ-5.10 | C-12, C-20 | Native tests; Acceptance checklist |
| REQ-5.11 | C-20 | Acceptance checklist |
| REQ-5.12 | C-20 | Acceptance checklist; maximum loop duration |
| REQ-6.1 | C-10 | Native tests |
| REQ-6.2 | C-12 | Native tests |
| REQ-6.3 | C-12 | Native tests |
| REQ-6.4 | C-10 | Native tests; Acceptance checklist (power off and on) |
| REQ-6.5 | C-10, C-21 | Native tests |
| REQ-6.6 | C-10 | Native tests |
| REQ-6.7 | C-10 | Native tests |
| REQ-6.8 | C-10, C-21 | Native tests |
| REQ-7.1 | C-13, C-17 | Acceptance checklist |
| REQ-7.2 | C-10, C-17, C-18 | Native tests; Acceptance checklist |
| REQ-7.3 | C-13, C-17 | Native tests; Acceptance checklist |
| REQ-7.4 | C-13, C-18 | Acceptance checklist |
| REQ-7.5 | C-13 | Native tests on `FakeLcd` |
| REQ-7.6 | C-9, C-13 | Native tests |
| REQ-7.7 | C-13, C-18 | Native tests |
| REQ-7.8 | C-9, C-13 | Native tests |
| REQ-7.9 | C-13, C-24 | Acceptance checklist |
| REQ-7.10 | C-13 | Native tests |
| REQ-7.11 | C-13, C-17, C-18 | Acceptance checklist (perceived scale in the dark) |
| REQ-7.12 | C-10 | Native tests |
| REQ-8.1 | C-12, C-14, C-19 | Native tests; Acceptance checklist |
| REQ-8.2 | C-14 | Native tests |
| REQ-8.3 | C-6, C-14 | Native tests |
| REQ-8.4 | C-14, C-19 | Native tests; Acceptance checklist |
| REQ-8.5 | C-14 | Native tests |
| REQ-8.6 | C-14, C-19 | Acceptance checklist (listening) |
| REQ-8.7 | C-14 | Native tests on `FakeBuzzer` |
| REQ-9.1 | C-15, C-24 | Acceptance checklist |
| REQ-9.2 | C-15 | Acceptance checklist |
| REQ-9.3 | C-10, C-15 | Native tests |
| REQ-9.4 | C-15, C-16, C-24 | Acceptance checklist |
| REQ-9.5 | C-15 | Acceptance checklist |
| REQ-9.6 | C-15, C-23 | Acceptance checklist with `pio device monitor` |
| REQ-9.7 | C-10, C-15 | Native tests |
| REQ-9.8 | C-10, C-15 | Native tests |
| REQ-9.9 | C-15, C-22 | Test build with a stuck loop |
| REQ-9.10 | C-15, C-23 | Acceptance checklist with `pio device monitor` |
| REQ-9.11 | C-10, C-15 | Native tests |
| REQ-9.12 | C-7, C-15 | Native tests |
| REQ-9.13 | C-29 | Test with a synthetic log; Acceptance checklist |
| REQ-9.14 | C-15 | Native tests |
| REQ-10.1 | C-17 | Acceptance checklist |
| REQ-10.2 | C-18 | Acceptance checklist |
| REQ-10.3 | C-2, C-4 | `pio run`; inspection of `platformio.ini` |
| REQ-10.4 | C-18, C-30 | Comparison with `PINOUT.md` |
| REQ-10.5 | C-27, C-30 | Wiring inspection |
| REQ-11.1 | C-4, C-5, C-26 | `grep` for Arduino includes in `lib/r080_core` |
| REQ-11.2 | C-5, C-24 | Code review |
| REQ-11.3 | C-2, C-6, C-25 | `pio test -e native` |
| REQ-11.4 | C-3 | Workflow on GitHub |
| REQ-11.5 | C-2, C-5 | Native tests |
| REQ-12.1 | C-27, C-28 | `CHANGELOG.md` and tag present |
| REQ-12.2 | C-28 | Signed checklist for the tag |
| REQ-12.3 | C-27 | Review |
| REQ-12.4 | C-1, C-26 | `git ls-remote` |
| REQ-12.5 | C-27 | Manual review |

## Open questions

No open questions: all of them were resolved in round 7 of the grilling session.

- **Q-1 — Reset cause and brown-out threshold:** resolved (factory threshold, watchdog only; ISP in the backlog). See C-22.
- **Q-2 — Sound parameters:** resolved. See C-14.
- **Q-3 — Brightness scales:** resolved. See C-10, C-17, C-18.
- **Q-4 — End of a paused step-by-step test strip:** resolved. See C-12.
- **Q-5 — What counts as an Exposure in the counters:** resolved. See C-15.
- **Q-6 — Relay during the bootloader:** resolved. See C-30.
- **Q-7 — Calibration accuracy:** resolved. See C-15, C-23, C-29.
- **Q-8 — Service menu navigation:** resolved in REQ-9.14. See C-15.
