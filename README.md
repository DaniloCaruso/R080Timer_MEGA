<p align="center">
  <img src="docs/assets/r080-banner.svg" alt="R080 Timer: darkroom enlarger timer on Arduino Mega 2560" width="100%">
</p>

<p align="center">
  <a href="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/firmware.yml?query=branch%3Afeature%2Farduino-mega-firmware"><img src="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/firmware.yml/badge.svg?branch=feature/arduino-mega-firmware" alt="Firmware CI"></a>
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/"><img src="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/pages.yml/badge.svg" alt="Site"></a>
  <img src="https://img.shields.io/badge/firmware-v2.0.0-a985ff?style=flat-square" alt="Firmware v2.0.0">
  <img src="https://img.shields.io/badge/board-Arduino%20Mega%202560-45e0ff?style=flat-square" alt="Arduino Mega 2560">
  <a href="LICENSE"><img src="https://img.shields.io/badge/license-CC0%201.0-ff5cc8?style=flat-square" alt="License CC0 1.0"></a>
</p>

<p align="center">
  <b><a href="https://danilocaruso.github.io/R080Timer_MEGA/">Site</a></b> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/pinout.html">Pinout</a> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/funzioni.html">Features and controls</a> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/flussi.html">Flows</a> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/installazione.html">Setup</a> ·
  <a href="README-IT.md">Italiano</a>
</p>

R080 Timer drives the lamp of a darkroom enlarger: it decides how long the light exposes the paper. It runs on an Arduino Mega 2560 with a red LCD, a 7-segment display, a 4x4 keypad, two encoders and one big main button. The time is counted by a hardware timer, so the lamp turns off to the millisecond.

## What it does

| Modes | Precision | Safety |
|---|---|---|
| **Focus** with automatic shut-off | **Seconds** or **F-Stop**, from 1/1 to 1/64 of a stop | Lamp off at power-on and on every reset |
| **Countdown** with Metronome | Offset in stops always on the LCD | Watchdog: lamp off within 250 ms |
| **Test strip**, continuous or step-by-step | Clock correction in ppm | The timer keeps working even without the LCD |

| Channels | Darkroom lights | Service menu |
|---|---|---|
| `A`, `B`, `C`: each holds a complete print setting | LCD always red, 8 brightness levels | Counters, system health, component tests |
| Saved in EEPROM, restored at power-on | LCD dimmed during the Exposure | Clock calibration, factory reset |

## Where the code lives

| Branch | Firmware | Status |
|---|---|---|
| [`feature/arduino-mega-firmware`](https://github.com/DaniloCaruso/R080Timer_MEGA/tree/feature/arduino-mega-firmware) | **v2**, C++ and PlatformIO on Arduino Mega 2560, rewritten from scratch with tests on the PC | in acceptance testing on the board |
| [`feature/esp32-micropython-firmware`](https://github.com/DaniloCaruso/R080Timer_MEGA/tree/feature/esp32-micropython-firmware) | MicroPython port for ESP32-WROOM-32 | on hold |
| `main` (this branch) | **v1**, the original firmware; it also hosts the site (`site/`) and the shared docs | reference |

The v2 manual, pinout and build instructions are in the [`feature/arduino-mega-firmware` README](https://github.com/DaniloCaruso/R080Timer_MEGA/tree/feature/arduino-mega-firmware#readme) and on the [site](https://danilocaruso.github.io/R080Timer_MEGA/).

## Quick start (v2)

```
git clone -b feature/arduino-mega-firmware https://github.com/DaniloCaruso/R080Timer_MEGA.git
cd R080Timer_MEGA
pio run -t upload            # build and upload to the Mega
pio test -e native           # logic tests on the PC
pio device monitor -b 115200 # event log
```

You need [PlatformIO](https://platformio.org/). After the first upload, calibrate the clock: see [Setup](https://danilocaruso.github.io/R080Timer_MEGA/installazione.html).

<details>
<summary><b>v1 firmware on this branch</b></summary>

The code on `main` is the original firmware, kept as a reference. Its wiring differs from v2 in one point: the 7-segment display is on **A2** (v2 moves it to D14 and adds a 10 kΩ resistor between D47 and GND).

| Component | Pin |
|---|---|
| LCD (I²C) | SDA 20, SCL 21 |
| 7-segment display | A2 |
| Keypad rows / columns | 33, 31, 29, 27 / 41, 39, 37, 35 |
| Mode encoder DT / CLK / SW | 3 / 4 / 17 |
| Precision encoder DT / CLK / SW | 18 / 19 / 16 |
| Main button | 2 |
| Buzzer | 49 |
| Enlarger relay | 47 |

Build with `pio run`, upload with `pio run -t upload`. Libraries: [EncoderButton](https://github.com/Stutchbury/EncoderButton), [Encoder](https://github.com/PaulStoffregen/Encoder), [Keypad](https://playground.arduino.cc/Code/Keypad/), [Waveshare LCD1602 RGB](https://www.waveshare.com/wiki/LCD1602_RGB_Module), [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL).

</details>

## Contributing

Pull requests are welcome. For major changes, please open an issue first to discuss what you would like to change.

## License

[CC0 1.0 Universal](LICENSE)
