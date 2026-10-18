# PINOUT — R080 Timer v2.0.0

Collegamenti e risorse dell'Arduino Mega 2560 (ATmega2560, 16 MHz). Fonte: `src/drivers/Pins.h` e i driver in `src/drivers/`.

## Modifiche rispetto alla v1

| Collegamento | v1 | v2.0.0 | Perché |
|---|---|---|---|
| 7 segmenti, ingresso dati del display | A2 (SoftwareSerial) | **D14** (TX3, seriale hardware Serial3) | SoftwareSerial disabilitava gli interrupt per circa 1 ms a byte |
| Relè, D47 | diretto al modulo | diretto al modulo, **più 10 kΩ tra D47 e GND** | tiene il relè spento finché il firmware non comanda il pin (bootloader compreso) |

Tutti gli altri collegamenti sono invariati. A2 è libero.

## Mappa dei pin

| Pin | Direzione | Componente | Note |
|---|---|---|---|
| D0 / D1 | USART0 | USB | Log degli eventi a 115200 baud |
| D2 | ingresso, pull-up | Pulsante principale | Chiude verso GND; antirimbalzo 20 ms |
| D3 | ingresso | Encoder della Modalità, DT | INT5 |
| D4 | ingresso | Encoder della Modalità, CLK | Senza interrupt, letto nella ISR di D3 |
| D14 | uscita (TX3) | 7 segmenti, RX del display | Serial3, 9600 baud |
| D15 | (RX3) | — | Riservato da Serial3, non collegato |
| D16 | ingresso, pull-up | Encoder della precisione, pulsante | Chiude verso GND |
| D17 | ingresso, pull-up | Encoder della Modalità, pulsante | Chiude verso GND |
| D18 | ingresso | Encoder della precisione, DT | INT3 |
| D19 | ingresso | Encoder della precisione, CLK | INT2 |
| D20 / D21 | I²C | LCD Waveshare 1602 RGB, SDA / SCL | 100 kHz, timeout 3 ms |
| D27, D29, D31, D33 | ingresso, pull-up | Tastierino, righe 4, 3, 2, 1 | |
| D35, D37, D39, D41 | uscita a scansione | Tastierino, colonne 4, 3, 2, 1 | |
| D47 (PL2) | uscita | Modulo relè della lampada | Attivo alto; 10 kΩ verso GND |
| D49 (PL0) | uscita | Buzzer | `tone()` su Timer2 |

## Componenti

**Relè.** D47 è il bit PL2 di PORTL. Il firmware lo mette in uscita a livello basso come prima istruzione di `setup()`; lo accende all'avvio di un'Esposizione (se l'Abilitazione relè del Canale è attiva) e lo spegne dalla ISR di Timer1 al tick esatto della fine o dello Step. La resistenza da 10 kΩ copre l'intervallo tra accensione e `setup()`, in cui il pin è un ingresso flottante; in parallelo a un'eventuale pull-down del modulo è innocua.

**7 segmenti.** Display seriale con i comandi `0x76` (reset), `0x7A` + byte (luminosità), `0x77` + byte (punti decimali). Il firmware scrive quattro cifre con il punto prima dell'ultima (`010.0`). Collegare solo l'ingresso RX del display a D14, più alimentazione e GND.

**LCD.** Waveshare LCD1602 RGB: controller del testo all'indirizzo I²C `0x3E`, retroilluminazione PCA9633 a `0x60`. Il firmware usa solo il canale rosso.

**Tastierino 4x4.**

| | D41 | D39 | D37 | D35 |
|---|---|---|---|---|
| **D33** | 1 | 2 | 3 | A |
| **D31** | 4 | 5 | 6 | B |
| **D29** | 7 | 8 | 9 | C |
| **D27** | * | 0 | # | D |

All'accensione il firmware legge `D` (D27 × D35) prima di inizializzare la libreria Keypad, per aprire il Menu di servizio.

**Encoder.** Una tacca vale quattro conteggi. I pulsanti degli encoder e il pulsante principale chiudono verso GND con il pull-up interno.

**Buzzer.** Pilotato con `tone()`: le frequenze dei suoni vanno da 150 a 1200 Hz.

## Risorse del microcontrollore

| Risorsa | Uso |
|---|---|
| Timer0 | `millis()` e `micros()` del core Arduino (suoni, LCD, salvataggio, log) |
| Timer1 | CTC a 1 kHz (prescaler 64, `OCR1A = 249`): unica sorgente del tempo di esposizione |
| Timer2 | `tone()` per il buzzer |
| Timer4 | Conteggio libero con prescaler 64, riferimento per contare i tick di Timer1 persi |
| Watchdog | Interrupt + reset, 250 ms per fase |
| USART0 | Log USB, 115200 baud |
| USART3 | 7 segmenti, 9600 baud |
| TWI | LCD, 100 kHz |
| EEPROM | Impostazioni a `0x000` (43 byte), contatori a `0x040` (17 byte) |
| INT2, INT3, INT5 | Encoder |

Per i timer occupati, `analogWrite()` non è utilizzabile su D6–D8 (Timer4), D9–D10 (Timer2) e D11–D13 (Timer1).

## Pin liberi

- Digitali: D5–D13, D22–D26, D28, D30, D32, D34, D36, D38, D40, D42–D46, D48, D50–D53.
- Analogici: A0–A15.
