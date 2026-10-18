# CLAUDE.md

Guida per agenti Claude Code. Per pinout, installazione e comportamento utente, vedere `README.md`.

---

## 1. Cos'è questo progetto

**R080 Timer** — firmware per timer da camera oscura (ingranditore fotografico). 3 modalità (Focus, Countdown, Test Strip), navigazione F-Stop, 3 canali con memoria, persistenza impostazioni, suoni non bloccanti.

Su questo branch (`feature/micropython-esp32-port`) è scritto in **MicroPython** per **ESP32-WROOM-32**. NON è Arduino, NON usa PlatformIO. Non generare mai codice C++/Arduino su questo branch: la versione Arduino vive su `main` e su `feature/rewrite_to_improve_stability_and_performance` (vedi §14).

Lo sviluppatore è italiano: stringhe UI e commenti in parte in italiano (`"PRONTO"`, `"Ch:A"`, ecc.). Preservarli. **Rispondere all'utente in italiano.**

---

## 2. Stack e toolchain

- **Runtime:** MicroPython (firmware ESP32_GENERIC da micropython.org)
- **Concorrenza:** `uasyncio` — niente thread, niente blocking sleep nel loop principale
- **Persistenza:** `esp32.NVS`, namespace `"r080"`, version chiave `1` (sostituisce il magic byte 0xA5 dell'EEPROM Arduino)
- **Test su host:** `pytest` + stub in `tests/stubs/` per `machine`, `esp32`, `uasyncio`
- **Deploy:** `mpremote` — NON PlatformIO

### Comandi make

```
make flash PORT=/dev/ttyUSB0 FW=esp32_generic.bin   # flash firmware (una volta)
make deploy PORT=/dev/ttyUSB0                        # copia .py sulla scheda
make repl PORT=/dev/ttyUSB0                          # REPL seriale
make test                                            # pytest su host
```

Su Linux: l'utente deve essere nel gruppo `dialout` per `/dev/ttyUSB*`.

---

## 3. Struttura dei moduli

```
boot.py                  boot ESP32
main.py                  uasyncio.run(App().run())

r080/
  config.py              pin, toni, tabelle (FSTOP_PRECISIONS, SECONDS_PRECISIONS)
  state.py               AppState — tutto lo stato applicativo
  fstop.py               FStopState: baseTime, stopOffset, calcolo
  stepmath.py            calcoli step incrementali
  operations.py          3 Operation (FOCUS, COUNTDOWN, TEST_STRIP)
  storage.py             NVS load/save
  sound.py               SoundPlayer (state machine non bloccante)
  engine.py              motore temporale asyncio (vedi §6)
  presentation.py        LCD 16x2 + display 7 segmenti
  inputs.py              keypad / encoder / pulsanti
  app.py                 orchestrazione e dispatch eventi

  hardware/
    relay.py, buzzer.py, seg7.py, lcd1602_rgb.py
    keypad.py, rotary.py, button.py

tests/
  stubs/                 stub machine/esp32/uasyncio
  test_*.py              test unitari (logica pura, no hardware)

tools/
  flash.sh, deploy.sh
```

---

## 4. Modello a moduli (NON single-translation-unit)

A differenza della versione Arduino (un unico `.cpp` con include ordinati), qui ogni file `.py` è un modulo importabile standard Python. Non ci sono vincoli sull'ordine di import, non ci sono definizioni globali condivise senza `import`, non ci sono header-only.

`AppState` in `state.py` raccoglie tutto lo stato. I moduli ricevono il riferimento allo stato come parametro, non accedono a globali impliciti.

---

## 5. Pin map (da `r080/config.py`)

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

Note hardware critiche:
- ESP32 è a 3.3 V: relay con trigger 3.3 V o transistor/level shifter.
- GPIO 34–39 (colonne keypad) sono solo-input, senza pull-up interni: servono 4 resistori da 10 kΩ a 3.3 V.

---

## 6. Engine temporale (`engine.py`)

`engine.py` sostituisce l'ISR Timer1 1 kHz dell'Arduino. Implementa un task `uasyncio` che:

- Mantiene un contatore ms monotono (equivalente di `g_tickMs`).
- In modalità COUNTDOWN: decrementa i ms rimanenti, spegne il relay alla scadenza tramite `machine.Timer` one-shot per precisione al ms, alza il flag `end_of_countdown`.
- In modalità TEST_STRIP: incrementa i ms elapsed, rileva il raggiungimento del prossimo step, alza `next_step_reached`.
- Ogni 100 ms: alza `tick_flag_100ms` per il ciclo di render.
- Ogni 1000 ms (countdown): alza `metronome_flag`.

I flag sono consumati dal loop principale in `app.py`. L'ordine di dispatch è lo stesso dell'Arduino: `end_of_countdown` → `next_step_reached` → `metronome_flag` → `tick_100ms` → render.

Non reintrodurre `time.sleep()` bloccanti nel path del loop. Usare `await uasyncio.sleep_ms()` o flag.

---

## 7. Invarianti fondamentali

### Unità di tempo: decisecondi

**Tutti i valori temporali sono in decisecondi (1/10 s).** `time_set = 100` → display mostra `10.0`. Range `0..9999`. I tempi F-stop calcolati sono clampati a `1..9999` (0 silenziare l'ingranditore non ha senso).

### Indicizzazione canali

`channel` è 1, 2 o 3 nello stato. Gli array sono 0-indexed: `channel_time[channel - 1]`, `channels_relay_activated[channel - 1]`. Non modificare questa convenzione — la persistenza NVS dipende dall'indice.

### Persistenza NVS

Namespace `"r080"`, chiave version `1` (sostituisce il magic byte 0xA5). `save_settings()` usa le API NVS che scrivono solo se il valore è cambiato. `load_settings()` verifica la versione; se assente o diversa, usa i valori di default e salva. Ogni mutazione che altera tempo, canale, mute o relay chiama `save_settings()`.

Estendere: aggiornare la chiave version a `2`, aggiungere chiave NVS, aggiornare `load_settings()` e `save_settings()` insieme.

### Non bloccante

- Niente `time.sleep()` nel loop principale (solo `await uasyncio.sleep_ms()`).
- Le sequenze sonore sono gestite da `SoundPlayer` (state machine). Non sostituire con sleep+tone.
- Il render LCD avviene ogni 100 ms su flag, non in busy-loop.

### Input solo a timer fermo

L'elaborazione di encoder e keypad avviene solo quando il timer è fermo (`is_action_started = False`). La rotazione degli encoder durante il countdown è intenzionalmente ignorata.

---

## 8. Modalità operative

### FOCUS

Conteggio crescente, relay ON. Keypad: `*`/`#` regolano luminosità 7-seg (1..10). Cifre ignorate. Nessuna precision/modality.

### COUNTDOWN

Conto alla rovescia. Metronomo ogni secondo. Relay spento all'arrivo a zero (one-shot timer). A fine corsa: `time_set = appo_time_set` (repeat-last-exposure), render a zero. Precision: SECONDS (default) o FSTOP. Nessuna modality.

### TEST STRIP

Conteggio crescente con step. Step beep a ogni traguardo. Warning beep nei 3 decisecondi precedenti allo step. Precision: SECONDS o FSTOP. Modality: CONTINOUS (relay sempre ON) o STEP_BY_STEP (relay OFF a ogni step, riparte al prossimo click).

---

## 9. F-Stop (`r080/fstop.py`)

`FStopState`: `base_time`, `stop_offset`. Formula: `base_time * 2 ** (stop_offset / precision_value)`, clampata `1..9999`. `precision_value` è il denominatore (1=intero, 2=mezzo, ...).

Dove `base_time` viene impostato (tenere coerente):
1. Ogni cifra keypad: `fstop_state.set_base(time_set)`.
2. Cambio canale: `set_base(time_set)`.
3. Switch SECONDS→FSTOP: `set_base(time_set if time_set > 0 else 1)`.
4. `*`/`#` in FSTOP con base zero: lazy-init.

Render offset: 4 caratteri (`"+02"` / `"-01"` / `" 00"`), LCD riga 1 colonne 5..7.

---

## 10. Sound (`r080/sound.py`)

`SoundPlayer` è una state machine. `emit_sound(freq, duration_ms)` accoda il suono. `tick_sound()` avanza la macchina a ogni iterazione del loop. `emit_sound()` è no-op quando `mute=True`. `emit_stop_sound()` non si avvia se una sequenza è già in corso.

Frequenze: `TONE_UP = 600 Hz` (conferme, step beep, warning); `TONE_DOWN = 300 Hz` (metronomo, stop sequence).

---

## 11. Display

### LCD 16x2 (Waveshare RGB, `r080/hardware/lcd1602_rgb.py`)

Driver I2C: indirizzo LCD 0x3E, indirizzo RGB 0x60. Backlight fisso rosso (`set_rgb(255, 0, 0)`). API: `init()`, `set_rgb()`, `custom_symbol()`, `set_cursor()`, `send_string()`, `write_char()`. Preferire `send_string`/`write_char`.

Layout riga 0: nome modalità + tipo precisione (`S`/`F` col 12) + modality (`CO`/`PP` col 14, solo TEST STRIP).
Layout riga 1: `Pr:<prec>` col 0..7, `Ch:X` col 9, ♪ col 14, ⚡ col 15. FSTOP offset col 5..7.

### 7-segmenti (`r080/hardware/seg7.py`)

UART2, TX GPIO 17, 9600 baud. Protocollo SparkFun seriale:
- `0x76` reset
- `0x77 0x04` punto decimale in posizione 1
- `0x7A <val>` luminosità (0..255)
- `"%04d" % v` → 4 cifre

`show_time(100)` mostra `10.0`. Non cambiare la maschera DP senza test su hardware reale.

---

## 12. Test

I moduli di logica pura (`fstop.py`, `stepmath.py`, `operations.py`, `storage.py`, `sound.py`, `engine.py`) sono testati su host con `pytest` + stub. I driver hardware (`hardware/`) e l'integrazione si verificano on-device.

Eseguire prima di ogni commit: `make test`. Non dichiarare che i test passano senza averli eseguiti.

---

## 13. Riferimenti comportamentali

Per il comportamento utente (cosa fa ogni tasto, ogni modalità, ogni encoder) vedere `README.md` e le sezioni di questo file. Il firmware è un port 1:1 del timer Arduino originale: la semantica di modalità, F-stop, canali e persistenza è invariata rispetto alla versione Arduino (storia consultabile su `main`).

---

## 14. Branch

`main` contiene il timer Arduino originale. Da `main` partono due linee di sviluppo indipendenti:

| Branch | Piattaforma | Etichetta issue |
|---|---|---|
| `feature/micropython-esp32-port` (questo) | MicroPython su ESP32-WROOM-32 | `esp32` |
| `feature/rewrite_to_improve_stability_and_performance` | C++/PlatformIO su Arduino Mega 2560 | `arduino` |

Le modifiche comuni (documentazione, configurazione agenti) vanno su `main`; i due branch vengono poi ribasati su `main`.

---

## Agent skills

### Issue tracker

Le issue vivono in GitHub Issues (`DaniloCaruso/R080Timer_MEGA`), gestite con la CLI `gh`. Ogni issue porta l'etichetta della piattaforma: su questo branch usare `esp32`. Vedere `docs/agents/issue-tracker.md`.

### Triage labels

Etichette predefinite: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. Vedere `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `GLOSSARY.md` alla radice. Niente ADR: le decisioni di ogni feature vanno in `docs/<nome-feature>/decisions.md`. Spec e piani precedenti a questa convenzione sono in `docs/superpowers/`. Vedere `docs/agents/domain.md`.
