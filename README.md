# R080 Timer

[Versione italiana](README-IT.md)

Darkroom enlarger timer on Arduino Mega 2560. Firmware v2.0.0, written in C++ with PlatformIO.

## Features

- Three Modes: Focus with automatic shut-off, Countdown, continuous or step-by-step Test strip.
- Precision in Seconds or F-Stop (1/1 to 1/64 of a stop), with the Offset shown on the LCD.
- Three Channels (`A`, `B`, `C`), each holding a complete print setting, saved in EEPROM.
- Time measured by Timer1 at 1 kHz: the lamp turns off to the millisecond, with clock correction in ppm.
- Darkroom lights: LCD always red, 8 brightness levels for LCD and 7-segment, LCD dimmed during the Exposure.
- Distinct sounds for start, Metronome, Pre-warning, Step, end; mute.
- Safety: relay off at power-on and on every reset, watchdog, debounce, timer working even without the LCD.
- Service menu: counters, system health, a test for every component, calibration, factory reset. Event log over USB.

## Hardware

Arduino Mega 2560, Waveshare 1602 RGB LCD (I²C), serial 7-segment display, 4x4 keypad, two rotary encoders with push button, main button, buzzer, relay module for the lamp.

| Component | Pin |
|---|---|
| LCD (I²C) | SDA D20, SCL D21 |
| 7-segment (display RX) | D14 (TX3) |
| Keypad rows / columns | D33, D31, D29, D27 / D41, D39, D37, D35 |
| Mode encoder DT / CLK / SW | D3 / D4 / D17 |
| Precision encoder DT / CLK / SW | D18 / D19 / D16 |
| Main button | D2 (to GND) |
| Buzzer | D49 |
| Lamp relay | D47, with a 10 kΩ resistor to GND |

Details in [PINOUT.md](PINOUT.md) (Italian).

**Upgrading from v1.** Move the 7-segment wire from A2 to D14 and add the 10 kΩ resistor between D47 and GND. At the first power-on the settings start from factory values.

## Build and upload

You need [PlatformIO](https://platformio.org/) (VS Code extension or PlatformIO Core).

```
pio run                      # build for the Mega
pio run -t upload            # build and upload to the board
pio test -e native           # logic tests on the PC
pio device monitor -b 115200 # event log
```

Libraries (fetched by PlatformIO): [Encoder](https://github.com/PaulStoffregen/Encoder), [Keypad](https://github.com/Chris--A/Keypad), [OneButton](https://github.com/mathertel/OneButton). The LCD has its own driver.

After the first upload, calibrate the clock: see the manual, §11.

## Documentation

- [DOCS.md](DOCS.md): user manual ([Italian](DOCS-IT.md))
- [PINOUT.md](PINOUT.md): wiring and microcontroller resources (Italian)
- [TECNICO.md](TECNICO.md): firmware architecture (Italian)
- [CHANGELOG.md](CHANGELOG.md): versions (Italian)
- [GLOSSARY.md](GLOSSARY.md): project vocabulary (Italian)
- `docs/firmware-rewrite/`: v2 requirements and decisions
- `tools/calibrate.py`: clock calibration from a PC

## License

[Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International](https://creativecommons.org/licenses/by-nc-sa/4.0/) (CC BY-NC-SA 4.0): you may share and adapt the project with attribution, not for commercial purposes, and under the same licence. Full text in [LICENSE](LICENSE).
