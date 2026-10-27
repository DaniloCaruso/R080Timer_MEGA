<p align="center">
  <img src="docs/assets/r080-banner.svg" alt="R080 Timer: timer per ingranditore da camera oscura su Arduino Mega 2560" width="100%">
</p>

<p align="center">
  <a href="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/firmware.yml?query=branch%3Afeature%2Farduino-mega-firmware"><img src="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/firmware.yml/badge.svg?branch=feature/arduino-mega-firmware" alt="CI del firmware"></a>
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/"><img src="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/pages.yml/badge.svg" alt="Sito"></a>
  <img src="https://img.shields.io/badge/firmware-v2.0.0-a985ff?style=flat-square" alt="Firmware v2.0.0">
  <img src="https://img.shields.io/badge/scheda-Arduino%20Mega%202560-45e0ff?style=flat-square" alt="Arduino Mega 2560">
  <a href="LICENSE"><img src="https://img.shields.io/badge/licenza-CC%20BY--NC--SA%204.0-ff5cc8?style=flat-square" alt="Licenza CC BY-NC-SA 4.0"></a>
</p>

<p align="center">
  <b><a href="https://danilocaruso.github.io/R080Timer_MEGA/">Sito</a></b> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/pinout">Pinout</a> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/funzioni">Funzioni e comandi</a> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/flussi">Flussi</a> ·
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/installazione">Installazione</a> ·
  <a href="README.md">English</a>
</p>

R080 Timer comanda la lampada dell'ingranditore in camera oscura: decide per quanto tempo la luce impressiona la carta. Gira su un Arduino Mega 2560 con un LCD rosso, un display a 7 segmenti, un tastierino 4x4, due encoder e un grande pulsante principale. Il tempo lo conta un timer hardware, quindi la lampada si spegne al millisecondo.

## Cosa fa

| Modalità | Precisione | Sicurezza |
|---|---|---|
| **Focus** con spegnimento automatico | **Secondi** o **F-Stop**, da 1/1 a 1/64 di stop | Lampada spenta all'accensione e a ogni reset |
| **Countdown** con Metronomo | Offset in stop sempre sull'LCD | Watchdog: lampada spenta entro 250 ms |
| **Test strip**, continuo o passo-passo | Correzione del clock in ppm | Il timer funziona anche senza LCD |

| Canali | Luci in camera oscura | Menu di servizio |
|---|---|---|
| `A`, `B`, `C`: ognuno tiene un'impostazione di stampa completa | LCD sempre rosso, 8 livelli di luminosità | Contatori, salute del sistema, test dei componenti |
| Salvati in EEPROM, ripristinati all'accensione | LCD attenuato durante l'Esposizione | Calibrazione del clock, ripristino di fabbrica |

## Dove sta il codice

| Branch | Firmware | Stato |
|---|---|---|
| [`feature/arduino-mega-firmware`](https://github.com/DaniloCaruso/R080Timer_MEGA/tree/feature/arduino-mega-firmware) | **v2**, C++ e PlatformIO su Arduino Mega 2560, riscritto da zero con test sul PC | in collaudo sulla scheda |
| [`feature/esp32-micropython-firmware`](https://github.com/DaniloCaruso/R080Timer_MEGA/tree/feature/esp32-micropython-firmware) | Port MicroPython per ESP32-WROOM-32 | in pausa |
| `main` (questo branch) | **v1**, il firmware originale; ospita anche il sito (`site/`) e i documenti comuni | riferimento |

Manuale, pinout e istruzioni di compilazione della v2 sono nel [README di `feature/arduino-mega-firmware`](https://github.com/DaniloCaruso/R080Timer_MEGA/blob/feature/arduino-mega-firmware/README-IT.md) e sul [sito](https://danilocaruso.github.io/R080Timer_MEGA/).

## Avvio rapido (v2)

```
git clone -b feature/arduino-mega-firmware https://github.com/DaniloCaruso/R080Timer_MEGA.git
cd R080Timer_MEGA
pio run -t upload            # compila e carica sul Mega
pio test -e native           # test della logica sul PC
pio device monitor -b 115200 # log degli eventi
```

Serve [PlatformIO](https://platformio.org/). Dopo il primo caricamento va calibrato il clock: vedi [Installazione](https://danilocaruso.github.io/R080Timer_MEGA/installazione).

<details>
<summary><b>Firmware v1 su questo branch</b></summary>

Il codice su `main` è il firmware originale, conservato come riferimento. Il cablaggio differisce dalla v2 in un punto: il display a 7 segmenti è su **A2** (la v2 lo sposta su D14 e aggiunge una resistenza da 10 kΩ tra D47 e GND).

| Componente | Pin |
|---|---|
| LCD (I²C) | SDA 20, SCL 21 |
| Display a 7 segmenti | A2 |
| Tastierino righe / colonne | 33, 31, 29, 27 / 41, 39, 37, 35 |
| Encoder Modalità DT / CLK / SW | 3 / 4 / 17 |
| Encoder precisione DT / CLK / SW | 18 / 19 / 16 |
| Pulsante principale | 2 |
| Buzzer | 49 |
| Relè dell'ingranditore | 47 |

Compilazione con `pio run`, caricamento con `pio run -t upload`. Librerie: [EncoderButton](https://github.com/Stutchbury/EncoderButton), [Encoder](https://github.com/PaulStoffregen/Encoder), [Keypad](https://playground.arduino.cc/Code/Keypad/), [Waveshare LCD1602 RGB](https://www.waveshare.com/wiki/LCD1602_RGB_Module), [ArduinoSTL](https://github.com/mike-matera/ArduinoSTL).

</details>

## Contribuire

Le pull request sono benvenute. Per modifiche importanti aprire prima una issue per discuterne.

## Licenza

[Creative Commons Attribuzione - Non commerciale - Condividi allo stesso modo 4.0 Internazionale](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.it) (CC BY-NC-SA 4.0): il progetto si può condividere e modificare citando l'autore, non a fini commerciali e con la stessa licenza. Testo completo in [LICENSE](LICENSE).
