<p align="center">
  <img src="docs/assets/r080-banner.svg" alt="R080 Timer: timer per ingranditore da camera oscura su Arduino Mega 2560" width="100%">
</p>

<p align="center">
  <a href="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/firmware.yml?query=branch%3Afeature%2Farduino-mega-firmware"><img src="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/firmware.yml/badge.svg?branch=feature/arduino-mega-firmware" alt="CI del firmware"></a>
  <a href="https://danilocaruso.github.io/R080Timer_MEGA/"><img src="https://github.com/DaniloCaruso/R080Timer_MEGA/actions/workflows/pages.yml/badge.svg" alt="Sito"></a>
  <img src="https://img.shields.io/badge/firmware-v2.1.0-a985ff?style=flat-square" alt="Firmware v2.1.0">
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

Timer per ingranditore da camera oscura su Arduino Mega 2560: decide per quanto tempo la luce impressiona la carta. Firmware v2.1.0, scritto in C++ con PlatformIO e testato sul PC. Il tempo lo conta il Timer1, quindi la lampada si spegne al millisecondo.

## Cosa fa

| Modalità | Precisione | Sicurezza |
|---|---|---|
| **Focus** con Spegnimento automatico | **Secondi** o **F-Stop**, da 1/1 a 1/64 di stop | Lampada spenta all'accensione e a ogni reset |
| **Countdown** con Metronomo | Offset in stop sempre sull'LCD | Watchdog: lampada spenta entro 250 ms |
| **Test strip**, continuo o passo-passo | Correzione del clock in ppm | Il timer funziona anche senza LCD |

| Canali | Luci e suoni in camera oscura | Menu di servizio |
|---|---|---|
| `A`, `B`, `C`: ognuno tiene un'impostazione di stampa completa | LCD sempre rosso, 8 livelli di luminosità per LCD e 7 segmenti | Contatori, salute del sistema, test di ogni componente |
| Salvati in EEPROM, ripristinati all'accensione | LCD attenuato durante l'Esposizione | Calibrazione del clock, ripristino di fabbrica, log degli eventi su USB |
| | Suoni distinti per avvio, Metronomo, Preavviso, Step, fine; mute | Testi dell'LCD in **italiano o inglese** (`Lingua/Language`) |

## Hardware

Arduino Mega 2560, LCD Waveshare 1602 RGB (I²C), display 7 segmenti seriale, tastierino 4x4, due encoder rotativi con pulsante, pulsante principale, buzzer, modulo relè per la lampada.

| Componente | Pin |
|---|---|
| LCD (I²C) | SDA D20, SCL D21 |
| 7 segmenti (RX del display) | D14 (TX3) |
| Tastierino righe / colonne | D33, D31, D29, D27 / D41, D39, D37, D35 |
| Encoder della Modalità DT / CLK / SW | D3 / D4 / D17 |
| Encoder della precisione DT / CLK / SW | D18 / D19 / D16 |
| Pulsante principale | D2 (verso GND) |
| Buzzer | D49 |
| Relè della lampada | D47, con resistenza da 10 kΩ verso GND |

Dettagli in [PINOUT.md](PINOUT.md).

**Aggiornamento.** Dalla v1: spostare il filo del 7 segmenti da A2 a D14 e aggiungere la resistenza da 10 kΩ tra D47 e GND. Dalla v1 o dalla v2.0.0: alla prima accensione le impostazioni ripartono dai valori di fabbrica (lingua italiana).

## Compilare e caricare

Serve [PlatformIO](https://platformio.org/) (estensione di VS Code o PlatformIO Core).

```
pio run                      # compila per il Mega
pio run -t upload            # compila e carica sulla scheda
pio test -e native           # test della logica sul PC
pio device monitor -b 115200 # log degli eventi
```

Librerie (scaricate da PlatformIO): [Encoder](https://github.com/PaulStoffregen/Encoder), [Keypad](https://github.com/Chris--A/Keypad), [OneButton](https://github.com/mathertel/OneButton). L'LCD ha un driver proprio.

Dopo il primo caricamento conviene calibrare il clock: vedi il manuale, §11.

## Documentazione

- [DOCS-IT.md](DOCS-IT.md): manuale d'uso ([inglese](DOCS.md))
- [PINOUT.md](PINOUT.md): collegamenti e risorse del microcontrollore
- [TECNICO.md](TECNICO.md): architettura del firmware
- [CHANGELOG.md](CHANGELOG.md): versioni
- [GLOSSARY.md](GLOSSARY.md): vocabolario del progetto
- `docs/firmware-rewrite/`, `docs/ui-languages/`: requisiti e decisioni della v2
- `tools/calibrate.py`: calibrazione del clock dal PC

## Licenza

[Creative Commons Attribuzione - Non commerciale - Condividi allo stesso modo 4.0 Internazionale](https://creativecommons.org/licenses/by-nc-sa/4.0/deed.it) (CC BY-NC-SA 4.0): il progetto si può condividere e modificare citando l'autore, non a fini commerciali e con la stessa licenza. Testo completo in [LICENSE](LICENSE).
