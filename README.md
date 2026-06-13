# R080 Timer — Timer per Ingranditore Fotografico

Timer digitale da camera oscura per ingranditore fotografico. Tre modalità operative (Focus, Countdown, Test Strip), navigazione F-Stop, 3 canali con memoria indipendente, persistenza delle impostazioni al riavvio, suoni non bloccanti.

Firmware scritto in **MicroPython** per **ESP32-WROOM-32**. Niente Arduino, niente PlatformIO.

---

## Requisiti hardware

### Scheda

**ESP32-WROOM-32 DevKit** (consigliata la versione a 38 pin; va bene anche la 30 pin perché tutti i 20 GPIO usati sono esposti su entrambe le versioni).

Tensione di lavoro: **3.3 V** — leggere attentamente le note critiche qui sotto prima di collegare i componenti.

### Mappa pin

| Funzione | GPIO |
|---|---|
| I2C SDA (LCD 0x3E + RGB 0x60) | 21 |
| I2C SCL | 22 |
| 7-seg TX (UART2) | 17 |
| Relay ingranditore | 23 |
| Buzzer (LEDC PWM) | 19 |
| Pulsante principale | 5 |
| Encoder Mode CLK / DT / SW | 16 / 4 / 18 |
| Encoder Precision CLK / DT / SW | 25 / 26 / 27 |
| Keypad righe 0..3 (output) | 13, 14, 32, 33 |
| Keypad colonne 0..3 (input) | 34, 35, 36, 39 |

### Note critiche

**Livelli logici 3.3 V.** L'ESP32 lavora a 3.3 V, non a 5 V. Il relay deve avere un ingresso trigger compatibile con 3.3 V (relay con transistor di pilotaggio integrato o modulo con optoisolatore). Per il display LCD Waveshare e il 7-segmenti seriale verificare la tensione di alimentazione e le linee di segnale: se il modulo accetta solo 5 V sulle linee dati, inserire un level shifter bidirezionale sulle linee I2C o un partitore su TX UART2.

**Pull-up esterni sul keypad.** I GPIO 34, 35, 36 e 39 (colonne keypad) sono pin solo-input e non hanno resistori di pull-up interni. Senza pull-up la scansione legge valori casuali. Collegare un resistore da 10 kΩ tra ciascuna colonna e il pin 3.3 V.

---

## Requisiti software

### Host (PC di sviluppo)

- Python 3.8 o superiore
- `esptool` — per flashare il firmware MicroPython sull'ESP32
- `mpremote` — per caricare i file .py sulla scheda e aprire il REPL
- `pytest` — per eseguire i test su host (con stub di `machine`, `esp32`, `uasyncio`)

Installare le dipendenze di sviluppo:

```
pip install -r requirements-dev.txt
```

Su Linux aggiungere il proprio utente al gruppo `dialout` per accedere a `/dev/ttyUSB*` e `/dev/ttyACM*` senza sudo:

```
sudo usermod -aG dialout $USER
```

(riavviare la sessione o eseguire `newgrp dialout` per rendere effettiva la modifica).

### Firmware ESP32

Scaricare il firmware **ESP32_GENERIC** da [micropython.org/download/ESP32_GENERIC](https://micropython.org/download/ESP32_GENERIC/). Non è necessario compilare nulla: si usa il file `.bin` precompilato.

---

## Installazione

### 1. Flash del firmware MicroPython (una volta sola)

```
make flash PORT=/dev/ttyUSB0 FW=esp32_generic.bin
```

Questo cancella la flash e installa MicroPython tramite `esptool`. Va fatto solo la prima volta o dopo un aggiornamento del firmware.

### 2. Deploy del codice applicativo

```
make deploy PORT=/dev/ttyUSB0
```

Copia `boot.py`, `main.py` e l'intera cartella `r080/` sulla scheda tramite `mpremote`.

### 3. REPL seriale

```
make repl PORT=/dev/ttyUSB0
```

Apre il terminale interattivo MicroPython (Ctrl-X per uscire con mpremote).

### 4. Test su host

```
make test
```

Esegue pytest usando gli stub in `tests/stubs/` al posto dei moduli hardware reali (`machine`, `esp32`, `uasyncio`). I driver e l'integrazione hardware si verificano on-device.

---

## Struttura del progetto

```
boot.py                  avvio ESP32 (configurazione di base)
main.py                  entrypoint: avvia App con uasyncio.run()
Makefile                 comandi flash / deploy / repl / test
requirements-dev.txt     dipendenze host (pytest, esptool, mpremote)

r080/
  config.py              costanti: pin, toni, tabelle precisioni, testi UI
  state.py               AppState: tutto lo stato applicativo
  fstop.py               FStopState: baseTime, stopOffset, calcolo F-stop
  stepmath.py            calcoli step incrementali (F-stop e secondi)
  operations.py          definizione e inizializzazione delle 3 operazioni
  storage.py             persistenza NVS (namespace "r080", version=1)
  sound.py               SoundPlayer: state machine non bloccante
  engine.py              motore temporale asyncio (sostituisce ISR Timer1)
  presentation.py        rendering LCD 16x2 e display 7 segmenti
  inputs.py              gestione keypad, encoder e pulsanti
  app.py                 orchestrazione: loop principale, dispatch eventi

  hardware/
    relay.py             driver relay (GPIO output)
    buzzer.py            driver buzzer (LEDC PWM)
    seg7.py              driver 7-seg seriale (UART2, protocollo SparkFun)
    lcd1602_rgb.py       driver LCD Waveshare RGB (I2C 0x3E + 0x60)
    keypad.py            scansione matrice 4x4
    rotary.py            decodifica encoder rotativo con pulsante
    button.py            gestione pulsante con debounce e long-press

tests/
  stubs/                 stub di machine, esp32, uasyncio per pytest su host
  test_*.py              test unitari dei moduli di logica pura

tools/
  flash.sh               script di supporto per esptool
  deploy.sh              script di supporto per mpremote
```

---

## Modalità operative

### Focus

Il timer conta in avanti con il relay ON. Serve per mettere a fuoco l'ingranditore a luce aperta. I tasti `*` e `#` regolano la luminosità del display 7 segmenti. Non ha precision né modality.

### Countdown

Conto alla rovescia dal tempo impostato a zero. Il relay si spegne al raggiungimento dello zero (spegnimento preciso, non soggetto al cadence di render). Un metronomo emette un beep ogni secondo durante il conteggio. A fine corsa il tempo torna al valore impostato originariamente (repeat-last-exposure), pronto per una nuova esposizione identica.

### Test Strip

Conteggio crescente con step automatici, per produrre una strip di test con esposizioni progressive. I step possono essere espressi in secondi o in F-stop (mezzo stop, terzo stop, ecc.). Modalità continua (il relay rimane ON tra uno step e l'altro) o passo-passo (il relay si ferma a ogni step e riparte al prossimo click del pulsante principale).

---

## Comandi keypad

| Tasto | Funzione |
|---|---|
| `A` / `B` / `C` | Cambia canale (1 / 2 / 3), ognuno con la propria memoria tempo |
| `D` | Reset del canale corrente (azzera il tempo) |
| `0`–`9` | Inserimento cifre del tempo (in decisecondi, max 4 cifre = 999.9 s) |
| `*` (click) | FOCUS: luminosità -1 / SECONDS: tempo -step / FSTOP: step down |
| `#` (click) | FOCUS: luminosità +1 / SECONDS: tempo +step / FSTOP: step up |
| `*` (hold) | Attiva/disattiva mute |
| `#` (hold) | Abilita/disabilita il relay del canale corrente |

Gli encoder di destra e sinistra selezionano rispettivamente la precisione (step di modifica del tempo) e la modalità operativa. I pulsanti degli encoder cambiano il tipo di precisione (SECONDS ↔ FSTOP) e la modalità test strip (CONTINOUS ↔ STEP_BY_STEP).

---

## Licenza

[CC0 1.0 Universal](LICENSE)
