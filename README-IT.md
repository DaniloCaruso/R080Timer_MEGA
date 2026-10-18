# R080 Timer

[English version](README.md)

Timer per ingranditore fotografico da camera oscura su Arduino Mega 2560. Firmware v2.0.0, scritto in C++ con PlatformIO.

## Funzioni

- Tre Modalità: Focus con Spegnimento automatico, Countdown, Test strip continuo o passo-passo.
- Precisione in Secondi o F-Stop (da 1/1 a 1/64 di stop), con Offset visibile sull'LCD.
- Tre Canali (`A`, `B`, `C`), ognuno con un'impostazione di stampa completa, salvati in EEPROM.
- Tempo misurato dal Timer1 a 1 kHz: la lampada si spegne al millisecondo, con Correzione del clock in ppm.
- Luci da camera oscura: LCD sempre rosso, 8 livelli di luminosità per LCD e 7 segmenti, LCD attenuato durante l'Esposizione.
- Suoni distinti per avvio, Metronomo, Preavviso, Step, fine; mute.
- Sicurezza: relè spento all'accensione e a ogni reset, watchdog, antirimbalzo, timer funzionante anche senza LCD.
- Menu di servizio: contatori, salute del sistema, test di ogni componente, calibrazione, ripristino di fabbrica. Log degli eventi su USB.

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

**Aggiornamento dalla v1.** Spostare il filo del 7 segmenti da A2 a D14 e aggiungere la resistenza da 10 kΩ tra D47 e GND. Alla prima accensione le impostazioni ripartono dai valori di fabbrica.

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
- `docs/firmware-rewrite/`: requisiti e decisioni della v2
- `tools/calibrate.py`: calibrazione del clock dal PC

## Licenza

[CC0 1.0 Universal](LICENSE)
