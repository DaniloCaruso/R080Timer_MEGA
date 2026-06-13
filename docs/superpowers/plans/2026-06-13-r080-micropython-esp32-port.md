# R080 Timer — Port MicroPython/ESP32 — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Riscrivere il firmware R080Timer_MEGA (timer da camera oscura, C++/Arduino Mega) come port MicroPython 1:1 su ESP32-WROOM-32, stessi componenti hardware.

**Architecture:** Pacchetto `r080/` con layer separati: logica pura (state/fstop/operations) testabile su host CPython con mock di `machine`/`esp32`; driver hardware; motore temporale `uasyncio` (sostituisce l'ISR Timer1 1 kHz) con one-shot `machine.Timer` per lo spegnimento relay preciso; persistenza `esp32.NVS`. Entrypoint asyncio in `app.py`.

**Tech Stack:** MicroPython (ESP32 port), `uasyncio`, `machine` (I2C/UART/Pin/PWM/Timer), `esp32.NVS`. Toolchain: `esptool` (flash firmware), `mpremote` (deploy `.py`). Test host: `pytest` su CPython con stub di `machine`/`esp32`/`uasyncio` in `tests/stubs/`.

**Riferimenti:** spec di design in `docs/superpowers/specs/2026-06-13-r080-micropython-esp32-port-design.md`. Sorgente C++ di riferimento (da consultare per parità comportamentale): `include/functions/{fstop,timer_isr,timer_manager,storage,sound,keypad_mgmt,encoder_mgmt,presentation}.h`, `include/types.h`, `include/variables.h`, `include/constants.h`, `lib/WaveShare-Lib/Waveshare_LCD1602_RGB.cpp`.

**Unità di tempo:** decisecondi (1/10 s) ovunque, range 0..9999, come l'originale. `timeSet=100` ⇒ display `10.0`.

---

## Mappa Pin (ESP32-WROOM-32 38-pin)

| Funzione | GPIO | | Funzione | GPIO |
|---|---|---|---|---|
| I2C SDA | 21 | | Enc Mode CLK/DT/SW | 16/4/18 |
| I2C SCL | 22 | | Enc Prec CLK/DT/SW | 25/26/27 |
| 7-seg TX (UART2) | 17 | | Keypad righe 0..3 (out) | 13,14,32,33 |
| Relay | 23 | | Keypad colonne 0..3 (in) | 34,35,36,39 (+pull-up est. 10k) |
| Buzzer (LEDC) | 19 | | | |
| Pulsante principale | 5 | | | |

---

## Strategia Git

Branch `feature/micropython-esp32-port` creato dal branch corrente. "Replace everything": si rimuovono i file Arduino e si costruisce l'albero MicroPython. Rebase su `main` in seguito (conflitti totali attesi → risolvere mantenendo il nuovo albero). Commit frequenti, uno per task.

---

## File Structure

```
boot.py                       # boot ESP32 (freq, gc)
main.py                       # entrypoint: uasyncio.run(app.run())
r080/
  __init__.py
  config.py                   # costanti: pin, toni, testi, keymap, indirizzi I2C, precision arrays
  state.py                    # AppState: tutti i globali mutabili
  fstop.py                    # FStopState (logica pura)
  stepmath.py                 # calcolo prossimo step test-strip + clamp (logica pura)
  operations.py               # Operation/OperationStatus + le 3 modalità
  storage.py                  # NVS load/save (iniettabile per test)
  sound.py                    # SoundPlayer non-bloccante (clock iniettabile)
  engine.py                   # TimingEngine asyncio + one-shot relay Timer
  presentation.py             # rendering LCD + 7-seg
  inputs.py                   # gestione keypad/encoder/pulsanti
  app.py                      # orchestrazione + task asyncio
  hardware/
    __init__.py
    relay.py                  # gate relay
    buzzer.py                 # tone() via LEDC
    seg7.py                   # 7-seg seriale
    lcd1602_rgb.py            # driver Waveshare
    keypad.py                 # scanner 4x4 + macchina a stati
    rotary.py                 # decoder quadratura + pulsante encoder
    button.py                 # OneButton-like
tests/
  conftest.py                 # inserisce tests/stubs nel path
  stubs/
    machine.py                # stub Pin/I2C/UART/PWM/Timer per CPython
    esp32.py                  # stub NVS in-memory
    uasyncio.py               # re-export di asyncio
  test_fstop.py
  test_stepmath.py
  test_operations.py
  test_storage.py
  test_sound.py
tools/
  flash.sh                    # esptool write_flash
  deploy.sh                   # mpremote cp + reset
Makefile                      # flash, deploy, repl, test, clean
README.md                     # adattato ESP32/MicroPython
CLAUDE.md                     # riscritto per nuovo stack
```

---

## FASE 0 — Branch, scaffolding, toolchain di test

### Task 0.1: Creare il branch e rimuovere i file Arduino

**Files:**
- Branch: `feature/micropython-esp32-port` da `HEAD`
- Remove: `src/`, `include/`, `lib/`, `platformio.ini`

- [ ] **Step 1: Creare il branch dal branch corrente**

Run:
```bash
git checkout -b feature/micropython-esp32-port
```
Expected: `Switched to a new branch 'feature/micropython-esp32-port'`

- [ ] **Step 2: Rimuovere i file specifici Arduino**

> NOTA: azione distruttiva approvata dall'utente ("replace everything"). I sorgenti C++ restano nella storia git e nel branch precedente.

Run:
```bash
git rm -r src include lib platformio.ini
```
Expected: elenco dei file rimossi, nessun errore.

- [ ] **Step 3: Commit**

```bash
git commit -m "chore: remove Arduino firmware for MicroPython rewrite"
```

### Task 0.2: Scaffolding pacchetto e stub di test host

**Files:**
- Create: `r080/__init__.py`, `r080/hardware/__init__.py` (vuoti)
- Create: `tests/conftest.py`, `tests/stubs/machine.py`, `tests/stubs/esp32.py`, `tests/stubs/uasyncio.py`
- Create: `requirements-dev.txt`

- [ ] **Step 1: Creare i package marker**

`r080/__init__.py` e `r080/hardware/__init__.py`: file vuoti.

- [ ] **Step 2: Creare `requirements-dev.txt`**

```
pytest>=7.0
```

- [ ] **Step 3: Creare `tests/conftest.py`** (rende importabili stub e pacchetto)

```python
import os
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
# Gli stub di machine/esp32/uasyncio hanno priorità sui moduli reali (assenti su CPython)
sys.path.insert(0, os.path.join(ROOT, "tests", "stubs"))
sys.path.insert(0, ROOT)
```

- [ ] **Step 4: Creare `tests/stubs/machine.py`** (stub hardware per CPython)

```python
"""Stub minimale di `machine` per i test host CPython."""


class Pin:
    IN = 0
    OUT = 1
    PULL_UP = 2
    IRQ_FALLING = 4
    IRQ_RISING = 8

    def __init__(self, id, mode=-1, pull=-1, value=None):
        self.id = id
        self.mode = mode
        self.pull = pull
        self._value = 0 if value is None else value
        self._handler = None

    def value(self, v=None):
        if v is None:
            return self._value
        self._value = int(v)

    def on(self):
        self._value = 1

    def off(self):
        self._value = 0

    def irq(self, handler=None, trigger=0):
        self._handler = handler


class I2C:
    def __init__(self, id, scl=None, sda=None, freq=400000):
        self.writes = []  # lista di (addr, bytes)

    def writeto(self, addr, buf):
        self.writes.append((addr, bytes(buf)))

    def writeto_mem(self, addr, memaddr, buf):
        self.writes.append((addr, memaddr, bytes(buf)))

    def scan(self):
        return [0x3E, 0x60]


class UART:
    def __init__(self, id, baudrate=9600, tx=None, rx=None):
        self.sent = bytearray()

    def write(self, buf):
        if isinstance(buf, str):
            buf = buf.encode()
        self.sent.extend(buf)


class PWM:
    def __init__(self, pin, freq=600, duty=0):
        self._freq = freq
        self._duty = duty

    def freq(self, f=None):
        if f is None:
            return self._freq
        self._freq = f

    def duty(self, d=None):
        if d is None:
            return self._duty
        self._duty = d

    def deinit(self):
        self._duty = 0


class Timer:
    ONE_SHOT = 0
    PERIODIC = 1

    def __init__(self, id):
        self.id = id
        self._cb = None

    def init(self, mode=0, period=0, callback=None):
        self._cb = callback

    def deinit(self):
        self._cb = None
```

- [ ] **Step 5: Creare `tests/stubs/esp32.py`** (NVS in-memory)

```python
"""Stub di `esp32.NVS` in-memory per i test host."""

_STORE = {}


class NVS:
    def __init__(self, namespace):
        self.ns = namespace
        _STORE.setdefault(namespace, {})

    def get_i32(self, key):
        d = _STORE[self.ns]
        if key not in d:
            raise OSError("key not found")
        return d[key]

    def set_i32(self, key, value):
        _STORE[self.ns][key] = int(value)

    def commit(self):
        pass


def _reset():
    _STORE.clear()
```

- [ ] **Step 6: Creare `tests/stubs/uasyncio.py`**

```python
from asyncio import *  # noqa: F401,F403
import asyncio as _a

sleep_ms = lambda ms: _a.sleep(ms / 1000)
```

- [ ] **Step 7: Verificare che pytest parta (nessun test ancora)**

Run: `python -m pytest -q`
Expected: `no tests ran` (exit 0/5), nessun errore di import.

- [ ] **Step 8: Commit**

```bash
git add r080/__init__.py r080/hardware/__init__.py tests requirements-dev.txt
git commit -m "chore: scaffold package and host test stubs"
```

---

## FASE 1 — Costanti e logica pura (TDD su host)

### Task 1.1: `config.py` — costanti e tabelle

**Files:**
- Create: `r080/config.py`

- [ ] **Step 1: Scrivere `r080/config.py`** (parità con `include/constants.h` e `include/types.h`)

```python
# --- Pin (ESP32-WROOM-32 38-pin) ---
I2C_SDA = 21
I2C_SCL = 22
SEG7_TX = 17
RELAY_PIN = 23
BUZZER_PIN = 19
MAIN_BTN_PIN = 5
ENC_MODE_CLK, ENC_MODE_DT, ENC_MODE_SW = 16, 4, 18
ENC_PREC_CLK, ENC_PREC_DT, ENC_PREC_SW = 25, 26, 27
KEYPAD_ROW_PINS = (13, 14, 32, 33)   # output, pilotate
KEYPAD_COL_PINS = (34, 35, 36, 39)   # input, pull-up esterni 10k

# --- I2C ---
LCD_ADDR = 0x3E
RGB_ADDR = 0x60

# --- Toni (Hz) ---
TONE_UP = 600
TONE_DOWN = 300

# --- Keymap 4x4 ---
KEYMAP = (
    ("1", "2", "3", "A"),
    ("4", "5", "6", "B"),
    ("7", "8", "9", "C"),
    ("*", "0", "#", "D"),
)

# --- Testi modalità ---
MODE_TEXTS = ("FOCUS", "COUNTDOWN", "TEST STRIP")

# --- Modalità / precisione (enum come interi) ---
FOCUS, COUNTDOWN, TEST_STRIP = 0, 1, 2
CONTINOUS, STEP_BY_STEP, NO_MODALITY = 0, 1, 2   # "CONTINOUS" voluto (parità originale)
FSTOP, SECONDS, NO_PRECISION = 0, 1, 2

PRECISION_COUNT = 11
# (valore, etichetta) — denominatori F-stop
FSTOP_PRECISIONS = (
    (1, "1/1"), (2, "1/2"), (3, "1/3"), (4, "1/4"), (6, "1/6"), (8, "1/8"),
    (12, "1/12"), (16, "1/16"), (24, "1/24"), (32, "1/32"), (64, "1/64"),
)
# (valore secondi, etichetta)
SECONDS_PRECISIONS = (
    (1, "1s"), (2, "2s"), (3, "3s"), (4, "4s"), (5, "5s"), (6, "6s"),
    (7, "7s"), (8, "8s"), (10, "10s"), (15, "15s"), (20, "20s"),
)

# --- Limiti tempo (decisecondi) ---
TIME_MIN = 0
TIME_MAX = 9999

# --- Glifi LCD custom ---
NOTE_GLYPH = (0x2, 0x3, 0x2, 0x2, 0xE, 0x1E, 0xC, 0x0)
SPINA_GLYPH = (0xA, 0xA, 0x1F, 0x11, 0x11, 0xE, 0x4, 0x4)

# --- Persistenza ---
NVS_NAMESPACE = "r080"
NVS_VERSION = 1
```

- [ ] **Step 2: Scrivere il test `tests/test_config.py`**

```python
from r080 import config


def test_precision_arrays_length():
    assert len(config.FSTOP_PRECISIONS) == config.PRECISION_COUNT
    assert len(config.SECONDS_PRECISIONS) == config.PRECISION_COUNT


def test_fstop_denominators():
    vals = [v for v, _ in config.FSTOP_PRECISIONS]
    assert vals == [1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64]


def test_seconds_steps():
    vals = [v for v, _ in config.SECONDS_PRECISIONS]
    assert vals == [1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20]


def test_keymap_shape():
    assert config.KEYMAP[3] == ("*", "0", "#", "D")
```

- [ ] **Step 3: Eseguire i test**

Run: `python -m pytest tests/test_config.py -v`
Expected: 4 PASSED.

- [ ] **Step 4: Commit**

```bash
git add r080/config.py tests/test_config.py
git commit -m "feat(config): pin map, tones, precision tables"
```

### Task 1.2: `fstop.py` — logica F-Stop (TDD)

**Files:**
- Create: `r080/fstop.py`, `tests/test_fstop.py`
- Riferimento parità: `include/functions/fstop.h`

- [ ] **Step 1: Scrivere il test `tests/test_fstop.py`**

```python
from r080.fstop import FStopState


def test_zero_base_short_circuits():
    s = FStopState()
    assert s.get_calculated_time(2) == 0  # base 0 → ritorna base


def test_full_stop_up_doubles():
    s = FStopState()
    s.set_base(100)        # 10.0 s
    s.step_up()            # +1 stop, denom 1 → ×2
    assert s.get_calculated_time(1) == 200


def test_full_stop_down_halves():
    s = FStopState()
    s.set_base(100)
    s.step_down()          # -1 stop, denom 1 → ÷2
    assert s.get_calculated_time(1) == 50


def test_half_stop_rounds():
    s = FStopState()
    s.set_base(100)
    s.step_up()            # +1 a denom 2 → ×2^(1/2)=1.4142 → 141.42 → 141
    assert s.get_calculated_time(2) == 141


def test_clamp_high():
    s = FStopState()
    s.set_base(9000)
    s.step_up()
    assert s.get_calculated_time(1) == 9999  # 18000 clampato


def test_clamp_low_never_zero():
    s = FStopState()
    s.set_base(1)
    s.step_down()
    s.step_down()
    assert s.get_calculated_time(1) == 1  # mai 0 (silenzierebbe l'ingranditore)


def test_set_base_resets_offset():
    s = FStopState()
    s.set_base(100)
    s.step_up()
    s.set_base(200)
    assert s.stop_offset == 0


def test_format_offset():
    s = FStopState()
    assert s.format_offset() == " 00"
    s.step_up(); s.step_up()
    assert s.format_offset() == "+02"
    s.set_base(100); s.step_down()
    assert s.format_offset() == "-01"
```

- [ ] **Step 2: Eseguire i test per verificarne il fallimento**

Run: `python -m pytest tests/test_fstop.py -v`
Expected: FAIL (ModuleNotFoundError: r080.fstop).

- [ ] **Step 3: Scrivere `r080/fstop.py`**

```python
def _constrain(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


class FStopState:
    def __init__(self, base_time=0, stop_offset=0):
        self.base_time = base_time
        self.stop_offset = stop_offset

    def reset(self):
        self.base_time = 0
        self.stop_offset = 0

    def set_base(self, time):
        self.base_time = time
        self.stop_offset = 0

    def get_calculated_time(self, precision_value):
        # precision_value = denominatore (1=full, 2=half, ...)
        if self.base_time <= 0 or precision_value <= 0:
            return self.base_time
        multiplier = 2.0 ** (self.stop_offset / precision_value)
        result = int(self.base_time * multiplier + 0.5)
        return _constrain(result, 1, 9999)

    def step_up(self):
        self.stop_offset += 1

    def step_down(self):
        self.stop_offset -= 1

    def format_offset(self):
        # 3 caratteri: "+02" / "-01" / " 00"
        if self.stop_offset > 0:
            return "+%02d" % min(self.stop_offset, 99)
        if self.stop_offset < 0:
            return "%03d" % max(self.stop_offset, -99)
        return " 00"
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_fstop.py -v`
Expected: 8 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/fstop.py tests/test_fstop.py
git commit -m "feat(fstop): F-stop calculation with TDD"
```

### Task 1.3: `stepmath.py` — calcolo prossimo step test-strip (TDD)

**Files:**
- Create: `r080/stepmath.py`, `tests/test_stepmath.py`
- Riferimento parità: `onTestStripStep()` in `include/functions/timer_manager.h`

- [ ] **Step 1: Scrivere il test `tests/test_stepmath.py`**

```python
from r080.stepmath import next_step_decis


def test_seconds_step_adds_value_times_ten():
    # SECONDS: nuovo = corrente + prec*10 (decis)
    assert next_step_decis(100, is_fstop=False, precision_value=5) == 150


def test_fstop_full_stop_doubles():
    # FSTOP: round(corrente * 2^(1/denom)); denom 1 → ×2
    assert next_step_decis(100, is_fstop=True, precision_value=1) == 200


def test_fstop_min_plus_one():
    # se l'incremento arrotonda allo stesso valore, forzare +1
    assert next_step_decis(1, is_fstop=True, precision_value=64) == 2


def test_clamp_high():
    assert next_step_decis(9000, is_fstop=False, precision_value=20) == 9999


def test_clamp_low():
    assert next_step_decis(0, is_fstop=False, precision_value=1) == 10
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_stepmath.py -v`
Expected: FAIL (ModuleNotFoundError).

- [ ] **Step 3: Scrivere `r080/stepmath.py`**

```python
def next_step_decis(current_decis, is_fstop, precision_value):
    if is_fstop:
        multiplier = 2.0 ** (1.0 / precision_value)
        new_target = int(current_decis * multiplier + 0.5)
        if new_target <= current_decis:
            new_target = current_decis + 1
    else:
        new_target = current_decis + precision_value * 10
    if new_target > 9999:
        new_target = 9999
    if new_target < 1:
        new_target = 1
    return new_target
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_stepmath.py -v`
Expected: 5 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/stepmath.py tests/test_stepmath.py
git commit -m "feat(stepmath): test-strip next-step computation with TDD"
```

### Task 1.4: `state.py` — stato applicativo

**Files:**
- Create: `r080/state.py`, `tests/test_state.py`
- Riferimento parità: `include/variables.h`

- [ ] **Step 1: Scrivere il test `tests/test_state.py`**

```python
from r080.state import AppState


def test_defaults():
    s = AppState()
    assert s.time_set == 0
    assert s.channel == 1
    assert s.brightness == 10
    assert s.mute is False
    assert s.channel_time == [0, 0, 0]
    assert s.channels_relay_activated == [True, True, True]


def test_current_channel_index_is_zero_based():
    s = AppState()
    s.channel = 3
    assert s.channel_index() == 2
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_state.py -v`
Expected: FAIL.

- [ ] **Step 3: Scrivere `r080/state.py`**

```python
from r080.fstop import FStopState


class AppState:
    def __init__(self):
        self.brightness = 10            # 1..10
        self.mute = False
        self.time_set = 0               # decisecondi correnti (7-seg)
        self.appo_time_set = 0          # backup per repeat-last-exposure
        self.n_cifra = 0                # cifre keypad in corso (0..3)
        self.channel = 1                # 1..3
        self.channel_time = [0, 0, 0]   # memoria per canale (decis)
        self.channels_relay_activated = [True, True, True]
        self.fstop = FStopState()
        self.is_main_btn_pressed = False
        self.is_action_started = False

    def channel_index(self):
        return self.channel - 1
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_state.py -v`
Expected: 2 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/state.py tests/test_state.py
git commit -m "feat(state): central AppState"
```

### Task 1.5: `operations.py` — modello modalità (TDD)

**Files:**
- Create: `r080/operations.py`, `tests/test_operations.py`
- Riferimento parità: `include/types.h` (Operation/OperationStatus), `include/functions/operations.h`, `encoder_mgmt.h`

- [ ] **Step 1: Scrivere il test `tests/test_operations.py`**

```python
from r080 import config
from r080.operations import build_operations, OperationStatus


def test_three_modes():
    ops = build_operations()
    assert [o.name for o in ops] == list(config.MODE_TEXTS)


def test_focus_has_no_precision_no_modality():
    ops = build_operations()
    focus = ops[config.FOCUS]
    assert focus.has_precision is False
    assert focus.has_modality is False


def test_countdown_default_seconds():
    ops = build_operations()
    cd = ops[config.COUNTDOWN]
    assert cd.has_precision is True
    assert cd.precision_type == config.SECONDS


def test_teststrip_default_continous_seconds():
    ops = build_operations()
    ts = ops[config.TEST_STRIP]
    assert ts.has_modality is True
    assert ts.modality == config.CONTINOUS
    assert ts.precision_type == config.SECONDS


def test_mode_index_wraps():
    st = OperationStatus(build_operations())
    st.select_by_encoder(3)   # (3 % 3) = 0 → FOCUS
    assert st.current.name == "FOCUS"
    st.select_by_encoder(-1)  # (-1 % 3 + 3) % 3 = 2 → TEST STRIP
    assert st.current.name == "TEST STRIP"


def test_precision_index_wraps_and_sets_value():
    st = OperationStatus(build_operations())
    st.select_by_encoder(config.COUNTDOWN)
    st.set_precision_by_encoder(8)   # SECONDS index 8 → valore 10
    assert st.current.precision_value() == 10


def test_toggle_precision_type():
    st = OperationStatus(build_operations())
    st.select_by_encoder(config.COUNTDOWN)
    assert st.current.precision_type == config.SECONDS
    st.current.toggle_precision_type()
    assert st.current.precision_type == config.FSTOP


def test_toggle_modality_only_teststrip():
    st = OperationStatus(build_operations())
    st.select_by_encoder(config.TEST_STRIP)
    st.current.toggle_modality()
    assert st.current.modality == config.STEP_BY_STEP
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_operations.py -v`
Expected: FAIL.

- [ ] **Step 3: Scrivere `r080/operations.py`**

```python
from r080 import config


class Operation:
    def __init__(self, name, has_precision=False, has_modality=False):
        self.name = name
        self.has_precision = has_precision
        self.has_modality = has_modality
        self.precision_type = config.SECONDS if has_precision else config.NO_PRECISION
        self.precision_index = 0
        self.modality = config.CONTINOUS if has_modality else config.NO_MODALITY

    def precision_table(self):
        if self.precision_type == config.FSTOP:
            return config.FSTOP_PRECISIONS
        return config.SECONDS_PRECISIONS

    def precision_value(self):
        if not self.has_precision:
            return 0
        return self.precision_table()[self.precision_index][0]

    def precision_label(self):
        if not self.has_precision:
            return ""
        return self.precision_table()[self.precision_index][1]

    def set_precision_index(self, idx):
        self.precision_index = idx % config.PRECISION_COUNT

    def toggle_precision_type(self):
        if not self.has_precision:
            return
        self.precision_type = (
            config.FSTOP if self.precision_type == config.SECONDS else config.SECONDS
        )

    def toggle_modality(self):
        if not self.has_modality:
            return
        self.modality = (
            config.STEP_BY_STEP if self.modality == config.CONTINOUS else config.CONTINOUS
        )


def build_operations():
    return [
        Operation(config.MODE_TEXTS[config.FOCUS]),
        Operation(config.MODE_TEXTS[config.COUNTDOWN], has_precision=True),
        Operation(config.MODE_TEXTS[config.TEST_STRIP], has_precision=True, has_modality=True),
    ]


class OperationStatus:
    def __init__(self, operations):
        self.operations = operations
        self.current = operations[config.FOCUS]
        self.last = operations[config.FOCUS]

    def select_by_encoder(self, raw_pos):
        idx = (raw_pos % 3 + 3) % 3
        self.last = self.current
        self.current = self.operations[idx]

    def set_precision_by_encoder(self, raw_pos):
        idx = (raw_pos % config.PRECISION_COUNT + config.PRECISION_COUNT) % config.PRECISION_COUNT
        self.current.set_precision_index(idx)
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_operations.py -v`
Expected: 8 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/operations.py tests/test_operations.py
git commit -m "feat(operations): mode/precision/modality model with TDD"
```

---

## FASE 2 — Persistenza NVS (TDD su host con stub)

### Task 2.1: `storage.py` — load/save NVS

**Files:**
- Create: `r080/storage.py`, `tests/test_storage.py`
- Riferimento parità: `include/functions/storage.h`

- [ ] **Step 1: Scrivere il test `tests/test_storage.py`**

```python
import esp32  # stub
from r080 import config
from r080.state import AppState
from r080.storage import Storage


def setup_function(_):
    esp32._reset()


def test_load_returns_false_on_first_boot():
    st = AppState()
    storage = Storage()
    assert storage.load(st) is False


def test_save_then_load_roundtrip():
    st = AppState()
    st.brightness = 7
    st.mute = True
    st.channel = 2
    st.channel_time = [120, 0, 999]
    st.channels_relay_activated = [True, False, True]
    Storage().save(st)

    st2 = AppState()
    assert Storage().load(st2) is True
    assert st2.brightness == 7
    assert st2.mute is True
    assert st2.channel == 2
    assert st2.channel_time == [120, 0, 999]
    assert st2.channels_relay_activated == [True, False, True]
    # time_set caricato dal canale corrente
    assert st2.time_set == 0          # canale 2 → channel_time[1]
    assert st2.appo_time_set == 0


def test_load_clamps_values():
    nvs = esp32.NVS(config.NVS_NAMESPACE)
    nvs.set_i32("version", config.NVS_VERSION)
    nvs.set_i32("brightness", 99)     # fuori range
    nvs.set_i32("mute", 0)
    nvs.set_i32("channel", 5)         # fuori range
    for i in range(3):
        nvs.set_i32("ch%d_time" % i, 50000)   # fuori range
        nvs.set_i32("ch%d_relay" % i, 1)
    st = AppState()
    assert Storage().load(st) is True
    assert st.brightness == 10
    assert st.channel == 3
    assert st.channel_time == [9999, 9999, 9999]
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_storage.py -v`
Expected: FAIL.

- [ ] **Step 3: Scrivere `r080/storage.py`**

```python
import esp32
from r080 import config


def _constrain(x, lo, hi):
    return lo if x < lo else hi if x > hi else x


class Storage:
    def __init__(self):
        self.nvs = esp32.NVS(config.NVS_NAMESPACE)

    def _get(self, key, default):
        try:
            return self.nvs.get_i32(key)
        except OSError:
            return default

    def _set_if_changed(self, key, value):
        # mirror della semantica eeprom_update: scrive solo se cambiato
        if self._get(key, None) != value:
            self.nvs.set_i32(key, value)

    def load(self, state):
        if self._get("version", None) != config.NVS_VERSION:
            return False
        state.brightness = _constrain(self._get("brightness", 10), 1, 10)
        state.mute = self._get("mute", 0) != 0
        state.channel = _constrain(self._get("channel", 1), 1, 3)
        for i in range(3):
            state.channel_time[i] = _constrain(self._get("ch%d_time" % i, 0), 0, 9999)
            state.channels_relay_activated[i] = self._get("ch%d_relay" % i, 1) != 0
        state.time_set = state.channel_time[state.channel - 1]
        state.appo_time_set = state.time_set
        return True

    def save(self, state):
        self._set_if_changed("version", config.NVS_VERSION)
        self._set_if_changed("brightness", int(state.brightness))
        self._set_if_changed("mute", 1 if state.mute else 0)
        self._set_if_changed("channel", int(state.channel))
        for i in range(3):
            self._set_if_changed("ch%d_time" % i, int(state.channel_time[i]))
            self._set_if_changed("ch%d_relay" % i, 1 if state.channels_relay_activated[i] else 0)
        self.nvs.commit()
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_storage.py -v`
Expected: 3 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/storage.py tests/test_storage.py
git commit -m "feat(storage): NVS persistence mirroring EEPROM layout"
```

---

## FASE 3 — Driver hardware (implementazione + verifica on-device)

> I driver toccano `machine`. Si testano gli aspetti puri (protocollo byte) con gli stub quando sensato, e si verifica il resto sull'ESP32 reale.

### Task 3.1: `hardware/relay.py`

**Files:**
- Create: `r080/hardware/relay.py`
- Riferimento parità: `relayStart()` in `include/functions/base.h`, macro in `timer_isr.h`

- [ ] **Step 1: Scrivere `r080/hardware/relay.py`**

```python
from machine import Pin
from r080 import config


class Relay:
    def __init__(self, pin=config.RELAY_PIN):
        self.pin = Pin(pin, Pin.OUT)
        self.pin.off()

    def on(self):
        self.pin.on()

    def off(self):
        self.pin.off()

    def set_gate(self, enabled):
        # gate per-canale: accende solo se abilitato (parità con relayStart)
        if enabled:
            self.pin.on()
        else:
            self.pin.off()
```

- [ ] **Step 2: Verifica on-device** (dopo che il deploy esiste, Fase 6)

Annotare come check manuale: con relay abilitato sul canale, `relay.on()` chiude il contatto.

- [ ] **Step 3: Commit**

```bash
git add r080/hardware/relay.py
git commit -m "feat(hw): relay gate driver"
```

### Task 3.2: `hardware/buzzer.py`

**Files:**
- Create: `r080/hardware/buzzer.py`
- Riferimento parità: `emitSound()` in `include/functions/sound.h`

- [ ] **Step 1: Scrivere `r080/hardware/buzzer.py`**

```python
from machine import Pin, PWM
from r080 import config


class Buzzer:
    def __init__(self, pin=config.BUZZER_PIN):
        self._pwm = PWM(Pin(pin), freq=config.TONE_UP, duty=0)
        self._pwm.duty(0)

    def tone(self, freq):
        # avvia un tono continuo; lo stop e' gestito dal SoundPlayer
        self._pwm.freq(freq)
        self._pwm.duty(512)   # 50% su scala 0..1023

    def no_tone(self):
        self._pwm.duty(0)
```

- [ ] **Step 2: Commit**

```bash
git add r080/hardware/buzzer.py
git commit -m "feat(hw): buzzer LEDC tone driver"
```

### Task 3.3: `hardware/seg7.py` — display 7 segmenti (test protocollo + on-device)

**Files:**
- Create: `r080/hardware/seg7.py`, `tests/test_seg7.py`
- Riferimento parità: `config7SegmentsDisplay`/`reset7SegmentsDisplay` in `config.h`, `showTime` in `presentation.h`

- [ ] **Step 1: Scrivere il test `tests/test_seg7.py`**

```python
import machine
from r080.hardware.seg7 import Seg7


def test_show_time_sends_dp_and_digits():
    uart = machine.UART(2, baudrate=9600, tx=17)
    seg = Seg7(uart)
    seg.show_time(100)   # 10.0
    # atteso: 0x77, maschera DP cifra1, poi "0100"
    assert uart.sent[0] == 0x77
    assert uart.sent[1] == 0b00000100
    assert bytes(uart.sent[2:6]) == b"0100"


def test_reset_sends_0x76():
    uart = machine.UART(2, baudrate=9600, tx=17)
    seg = Seg7(uart)
    seg.reset()
    assert uart.sent[0] == 0x76


def test_set_brightness():
    uart = machine.UART(2, baudrate=9600, tx=17)
    seg = Seg7(uart)
    uart.sent = bytearray()
    seg.set_brightness(7)
    assert uart.sent[0] == 0x7A
    assert uart.sent[1] == 7
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_seg7.py -v`
Expected: FAIL.

- [ ] **Step 3: Scrivere `r080/hardware/seg7.py`**

```python
class Seg7:
    """Display seriale stile SparkFun, 9600 baud, su UART iniettabile."""

    CMD_RESET = 0x76
    CMD_DP = 0x77
    CMD_BRIGHTNESS = 0x7A
    DP_MASK_POS1 = 0b00000100

    def __init__(self, uart):
        self.uart = uart

    def reset(self):
        self.uart.write(bytes([self.CMD_RESET]))

    def set_brightness(self, value):
        self.uart.write(bytes([self.CMD_BRIGHTNESS, value & 0xFF]))

    def show_time(self, decis):
        decis = 0 if decis < 0 else 9999 if decis > 9999 else decis
        self.uart.write(bytes([self.CMD_DP, self.DP_MASK_POS1]))
        self.uart.write(("%04d" % decis).encode())
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_seg7.py -v`
Expected: 3 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/hardware/seg7.py tests/test_seg7.py
git commit -m "feat(hw): 7-segment serial driver with protocol tests"
```

### Task 3.4: `hardware/lcd1602_rgb.py` — driver Waveshare (port 1:1)

**Files:**
- Create: `r080/hardware/lcd1602_rgb.py`
- Riferimento parità: `lib/WaveShare-Lib/Waveshare_LCD1602_RGB.cpp` (sequenza init e comandi esatti)

- [ ] **Step 1: Scrivere `r080/hardware/lcd1602_rgb.py`**

```python
import time
from machine import Pin, I2C
from r080 import config

# Comandi LCD
LCD_CLEARDISPLAY = 0x01
LCD_RETURNHOME = 0x02
LCD_ENTRYMODESET = 0x04
LCD_DISPLAYCONTROL = 0x08
LCD_FUNCTIONSET = 0x20
LCD_SETCGRAMADDR = 0x40
LCD_SETDDRAMADDR = 0x80
# flag
LCD_DISPLAYON = 0x04
LCD_ENTRYLEFT = 0x02
LCD_4BITMODE = 0x00
LCD_1LINE = 0x00
LCD_5x8DOTS = 0x00
# registri RGB
REG_MODE1 = 0x00
REG_MODE2 = 0x01
REG_RED = 0x04
REG_GREEN = 0x03
REG_BLUE = 0x02
REG_OUTPUT = 0x08


class LCD1602RGB:
    def __init__(self, i2c=None):
        self.i2c = i2c or I2C(0, scl=Pin(config.I2C_SCL), sda=Pin(config.I2C_SDA), freq=400000)

    def _command(self, value):
        self.i2c.writeto(config.LCD_ADDR, bytes([0x80, value]))

    def _write_data(self, value):
        self.i2c.writeto(config.LCD_ADDR, bytes([0x40, value]))

    def _set_reg(self, reg, data):
        self.i2c.writeto(config.RGB_ADDR, bytes([reg, data]))

    def init(self):
        showfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS
        time.sleep_ms(50)
        for _ in range(3):
            self._command(LCD_FUNCTIONSET | showfunction)
            time.sleep_ms(5)
        self._command(LCD_DISPLAYCONTROL | LCD_DISPLAYON)
        self.clear()
        self._command(LCD_ENTRYMODESET | LCD_ENTRYLEFT)
        # RGB
        self._set_reg(REG_MODE1, 0x00)
        self._set_reg(REG_OUTPUT, 0xFF)
        self._set_reg(REG_MODE2, 0x20)
        self.set_rgb(255, 255, 255)

    def clear(self):
        self._command(LCD_CLEARDISPLAY)
        time.sleep_ms(2)

    def set_rgb(self, r, g, b):
        self._set_reg(REG_RED, r)
        self._set_reg(REG_GREEN, g)
        self._set_reg(REG_BLUE, b)

    def set_cursor(self, col, row):
        addr = col | (0x80 if row == 0 else 0xC0)
        self._command(addr)

    def write_char(self, value):
        self._write_data(value)

    def send_string(self, s):
        for ch in s:
            self._write_data(ord(ch))

    def custom_symbol(self, location, charmap):
        location &= 0x7
        self._command(LCD_SETCGRAMADDR | (location << 3))
        for b in charmap:
            self._write_data(b)
```

- [ ] **Step 2: Verifica on-device** (Fase 6): `init()` + `set_rgb(255,0,0)` → retroilluminazione rossa; `send_string("PRONTO")` visibile.

- [ ] **Step 3: Commit**

```bash
git add r080/hardware/lcd1602_rgb.py
git commit -m "feat(hw): Waveshare 1602 RGB I2C driver port"
```

### Task 3.5: `hardware/button.py` — pulsante stile OneButton

**Files:**
- Create: `r080/hardware/button.py`
- Riferimento parità: `configureMainButton`/`onMainButtonClick` in `running_mgmt.h`

- [ ] **Step 1: Scrivere `r080/hardware/button.py`**

```python
import time
from machine import Pin


class Button:
    """Pulsante attivo-basso con debounce. update() va chiamato spesso."""

    def __init__(self, pin, debounce_ms=50, click_ms=100, press_ms=1000):
        self.pin = Pin(pin, Pin.IN, Pin.PULL_UP)
        self.debounce_ms = debounce_ms
        self.click_ms = click_ms
        self.press_ms = press_ms
        self._last_level = 1
        self._changed_at = time.ticks_ms()
        self._down_at = 0
        self.on_click = None
        self.on_long_press = None

    def update(self):
        level = self.pin.value()
        now = time.ticks_ms()
        if level != self._last_level:
            if time.ticks_diff(now, self._changed_at) < self.debounce_ms:
                return
            self._changed_at = now
            if level == 0:               # premuto (attivo-basso)
                self._down_at = now
            else:                        # rilasciato
                held = time.ticks_diff(now, self._down_at)
                if held >= self.press_ms and self.on_long_press:
                    self.on_long_press()
                elif held >= self.click_ms and self.on_click:
                    self.on_click()
            self._last_level = level
```

- [ ] **Step 2: Commit**

```bash
git add r080/hardware/button.py
git commit -m "feat(hw): OneButton-like debounced button"
```

### Task 3.6: `hardware/rotary.py` — encoder quadratura + pulsante

**Files:**
- Create: `r080/hardware/rotary.py`
- Riferimento parità: uso di `Encoder.read()>>2` in `encoder_mgmt.h`

- [ ] **Step 1: Scrivere `r080/hardware/rotary.py`**

```python
from machine import Pin


class Rotary:
    """Encoder a quadratura: IRQ su CLK, legge DT. position() ~ read()>>2 dell'originale."""

    def __init__(self, clk_pin, dt_pin):
        self.clk = Pin(clk_pin, Pin.IN, Pin.PULL_UP)
        self.dt = Pin(dt_pin, Pin.IN, Pin.PULL_UP)
        self._pos = 0
        self.clk.irq(handler=self._on_clk, trigger=Pin.IRQ_FALLING)

    def _on_clk(self, _pin):
        # un detent per fronte di discesa del CLK
        if self.dt.value():
            self._pos += 1
        else:
            self._pos -= 1

    def position(self):
        return self._pos
```

- [ ] **Step 2: Commit**

```bash
git add r080/hardware/rotary.py
git commit -m "feat(hw): quadrature rotary encoder driver"
```

### Task 3.7: `hardware/keypad.py` — scanner 4x4 con macchina a stati

**Files:**
- Create: `r080/hardware/keypad.py`, `tests/test_keypad.py`
- Riferimento parità: `evaluateKey()` in `keypad_mgmt.h` (stati HOLD/PRESSED/RELEASED, debounce 50ms, hold time)

- [ ] **Step 1: Scrivere il test della logica di stato `tests/test_keypad.py`**

> Si testa la macchina a stati con un clock e una funzione di scan iniettati (niente hardware).

```python
from r080.hardware.keypad import KeyStateMachine, IDLE, PRESSED, HOLD, RELEASED


def test_press_then_hold_then_release():
    clock = [0]
    sm = KeyStateMachine(debounce_ms=50, hold_ms=500, clock=lambda: clock[0])

    # tasto giù
    assert sm.update("5", True) == PRESSED
    clock[0] = 30
    assert sm.update("5", True) is None     # entro debounce, nessun nuovo evento
    clock[0] = 600
    assert sm.update("5", True) == HOLD      # superato hold_ms
    clock[0] = 650
    assert sm.update("5", False) == RELEASED


def test_quick_tap_is_released_not_hold():
    clock = [0]
    sm = KeyStateMachine(debounce_ms=50, hold_ms=500, clock=lambda: clock[0])
    assert sm.update("A", True) == PRESSED
    clock[0] = 100
    assert sm.update("A", False) == RELEASED
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_keypad.py -v`
Expected: FAIL.

- [ ] **Step 3: Scrivere `r080/hardware/keypad.py`**

```python
import time
from machine import Pin
from r080 import config

IDLE = 0
PRESSED = 1
HOLD = 2
RELEASED = 3


class KeyStateMachine:
    """Macchina a stati per singolo tasto, parità con Chris-A Keypad."""

    def __init__(self, debounce_ms=50, hold_ms=500, clock=time.ticks_ms):
        self.debounce_ms = debounce_ms
        self.hold_ms = hold_ms
        self._clock = clock
        self.state = IDLE
        self._down_at = 0
        self._last_change = 0

    def update(self, key, is_down):
        now = self._clock()
        if is_down and self.state == IDLE:
            self.state = PRESSED
            self._down_at = now
            self._last_change = now
            return PRESSED
        if is_down and self.state == PRESSED:
            if now - self._down_at >= self.hold_ms:
                self.state = HOLD
                return HOLD
            return None
        if (not is_down) and self.state in (PRESSED, HOLD):
            if now - self._last_change < self.debounce_ms and self.state == PRESSED:
                pass
            self.state = IDLE
            return RELEASED
        return None


class Keypad:
    """Scanner 4x4: righe pilotate, colonne lette (pull-up esterni su 34-39)."""

    def __init__(self, clock=time.ticks_ms):
        self.rows = [Pin(p, Pin.OUT, value=1) for p in config.KEYPAD_ROW_PINS]
        self.cols = [Pin(p, Pin.IN) for p in config.KEYPAD_COL_PINS]
        self.keymap = config.KEYMAP
        self._sm = {}
        self._clock = clock

    def _scan_down_key(self):
        # ritorna il primo tasto premuto (riga attiva-bassa)
        for r, row in enumerate(self.rows):
            row.value(0)
            for c, col in enumerate(self.cols):
                pressed = (col.value() == 0)
                if pressed:
                    row.value(1)
                    return self.keymap[r][c]
            row.value(1)
        return None

    def poll(self):
        """Ritorna (key, event) oppure (None, None)."""
        key = self._scan_down_key()
        if key is not None:
            sm = self._sm.setdefault(key, KeyStateMachine(clock=self._clock))
            ev = sm.update(key, True)
            return (key, ev) if ev is not None else (None, None)
        # nessun tasto giù: chiudi eventuali stati aperti
        for k, sm in self._sm.items():
            ev = sm.update(k, False)
            if ev is not None:
                return (k, ev)
        return (None, None)
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_keypad.py -v`
Expected: 2 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/hardware/keypad.py tests/test_keypad.py
git commit -m "feat(hw): 4x4 keypad scanner with state machine"
```

---

## FASE 4 — Suono e motore temporale

### Task 4.1: `sound.py` — SoundPlayer non-bloccante (TDD con clock finto)

**Files:**
- Create: `r080/sound.py`, `tests/test_sound.py`
- Riferimento parità: `SoundPlayer`/`tickSound`/`emitStopSound` in `include/functions/sound.h`

- [ ] **Step 1: Scrivere il test `tests/test_sound.py`**

```python
from r080 import config
from r080.sound import SoundPlayer


class FakeBuzzer:
    def __init__(self):
        self.events = []   # ("tone", freq) | ("off",)

    def tone(self, freq):
        self.events.append(("tone", freq))

    def no_tone(self):
        self.events.append(("off",))


def test_single_beep_starts_and_stops():
    clock = [0]
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: clock[0])
    sp.emit(config.TONE_UP, 150)
    assert ("tone", config.TONE_UP) in bz.events
    clock[0] = 160
    sp.tick()
    assert bz.events[-1] == ("off",)


def test_muted_emits_nothing():
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: 0, muted=lambda: True)
    sp.emit(config.TONE_UP, 150)
    assert bz.events == []


def test_stop_sequence_plays_five_events():
    clock = [0]
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: clock[0])
    sp.emit_stop()
    tones = 0
    for t in range(0, 700, 10):
        clock[0] = t
        sp.tick()
    tone_events = [e for e in bz.events if e[0] == "tone"]
    assert len(tone_events) == 3   # 3 beep TONE_DOWN nella sequenza


def test_stop_refused_while_active():
    clock = [0]
    bz = FakeBuzzer()
    sp = SoundPlayer(bz, clock=lambda: clock[0])
    sp.emit_stop()
    n = len(bz.events)
    sp.emit_stop()   # rifiutato: sequenza in corso
    assert len(bz.events) == n
```

- [ ] **Step 2: Eseguire i test (fallimento atteso)**

Run: `python -m pytest tests/test_sound.py -v`
Expected: FAIL.

- [ ] **Step 3: Scrivere `r080/sound.py`**

```python
import time
from r080 import config

# (frequenza, durata_ms); freq 0 = silenzio
STOP_SEQUENCE = (
    (config.TONE_DOWN, 75), (0, 150), (config.TONE_DOWN, 75), (0, 150), (config.TONE_DOWN, 150),
)


class SoundPlayer:
    def __init__(self, buzzer, clock=time.ticks_ms, muted=None):
        self.buzzer = buzzer
        self._clock = clock
        self._muted = muted or (lambda: False)
        self._seq = None
        self._index = 0
        self._started_at = 0
        self._active = False

    def _start_event(self, evt):
        freq, _ = evt
        if freq > 0 and not self._muted():
            self.buzzer.tone(freq)
        else:
            self.buzzer.no_tone()

    def emit(self, freq, duration_ms):
        # beep singolo non-bloccante: sequenza di un evento
        if self._muted():
            return
        self._seq = ((freq, duration_ms),)
        self._index = 0
        self._started_at = self._clock()
        self._active = True
        self._start_event(self._seq[0])

    def emit_stop(self):
        if self._active:
            return
        self._seq = STOP_SEQUENCE
        self._index = 0
        self._started_at = self._clock()
        self._active = True
        self._start_event(self._seq[0])

    def tick(self):
        if not self._active:
            return
        now = self._clock()
        if now - self._started_at < self._seq[self._index][1]:
            return
        self._index += 1
        if self._index >= len(self._seq):
            self._active = False
            self.buzzer.no_tone()
            return
        self._started_at = now
        self._start_event(self._seq[self._index])
```

- [ ] **Step 4: Eseguire i test**

Run: `python -m pytest tests/test_sound.py -v`
Expected: 4 PASSED.

- [ ] **Step 5: Commit**

```bash
git add r080/sound.py tests/test_sound.py
git commit -m "feat(sound): non-blocking SoundPlayer with TDD"
```

### Task 4.2: `engine.py` — motore temporale asyncio

**Files:**
- Create: `r080/engine.py`
- Riferimento parità: `timer_isr.h` (ISR) + `timer_manager.h` (start<mode>Timer, onTick100ms, onCountdownEnd, onTestStripStep, onMetronomeBeat)

> L'engine sostituisce l'ISR 1 kHz. La risoluzione utile è il decisecondo; il render gira a 100 ms.
> Lo spegnimento relay a fine countdown è garantito da un one-shot `machine.Timer` armato all'avvio
> (precisione < 1 ms), in più al controllo nel loop asyncio.

- [ ] **Step 1: Scrivere `r080/engine.py`**

```python
import time
import uasyncio as asyncio
from machine import Timer
from r080 import config
from r080.stepmath import next_step_decis

MODE_NONE = 0
MODE_FOCUS = 1
MODE_COUNTDOWN = 2
MODE_TEST_STRIP = 3


class TimingEngine:
    """
    Callbacks iniettati dall'app:
      render(decis)            -> aggiorna LCD/7-seg
      on_metronome()           -> beep metronomo (countdown, ogni secondo)
      on_countdown_end()       -> cleanup fine countdown
      on_step(new_target)      -> beep step test-strip + UI
      on_step_warning()        -> beep di avviso (1..3 decis prima dello step)
      relay_off()              -> spegne relay (gate gia' valutato)
    """

    def __init__(self, render, on_metronome, on_countdown_end, on_step,
                 on_step_warning, relay_off):
        self.render = render
        self.on_metronome = on_metronome
        self.on_countdown_end = on_countdown_end
        self.on_step = on_step
        self.on_step_warning = on_step_warning
        self.relay_off = relay_off

        self.mode = MODE_NONE
        self.current_decis = 0
        self.next_step_target = 0
        self._is_fstop = False
        self._precision_value = 1
        self._modality_step = False

        self._end_ticks = 0
        self._last_second = -1
        self._relay_timer = Timer(0)

    # --- avvio modalità (parità con start<mode>Timer) ---

    def start_focus(self):
        self.mode = MODE_FOCUS
        self.current_decis = 0

    def start_countdown(self, appo_decis, gate_enabled):
        if appo_decis <= 0:
            return
        self.mode = MODE_COUNTDOWN
        self.current_decis = appo_decis
        self._end_ticks = time.ticks_add(time.ticks_ms(), appo_decis * 100)
        self._last_second = -1
        if gate_enabled:
            # one-shot per spegnimento relay preciso a fine corsa
            self._relay_timer.init(mode=Timer.ONE_SHOT, period=appo_decis * 100,
                                   callback=lambda t: self.relay_off())

    def start_test_strip(self, appo_decis, is_fstop, precision_value, modality_step, resume):
        self._is_fstop = is_fstop
        self._precision_value = precision_value
        self._modality_step = modality_step
        self.mode = MODE_TEST_STRIP
        if not resume:
            self.current_decis = 0
            self.next_step_target = appo_decis if appo_decis > 0 else 10
        self._step_start_ticks = time.ticks_ms()
        self._step_base_decis = self.current_decis

    def stop(self):
        self.mode = MODE_NONE
        self._relay_timer.deinit()

    # --- task asyncio ---

    async def run_render(self):
        while True:
            await asyncio.sleep_ms(100)
            self._tick_100ms()

    def _tick_100ms(self):
        if self.mode == MODE_FOCUS:
            self.current_decis += 1
            self.render(self.current_decis)
        elif self.mode == MODE_COUNTDOWN:
            rem_ms = time.ticks_diff(self._end_ticks, time.ticks_ms())
            if rem_ms <= 0:
                self.relay_off()
                self.mode = MODE_NONE
                self._relay_timer.deinit()
                self.render(0)
                self.on_countdown_end()
                return
            decis = rem_ms // 100
            self.current_decis = decis
            self.render(decis)
            sec = rem_ms // 1000
            if sec != self._last_second:
                self._last_second = sec
                self.on_metronome()
        elif self.mode == MODE_TEST_STRIP:
            elapsed_ms = time.ticks_diff(time.ticks_ms(), self._step_start_ticks)
            self.current_decis = self._step_base_decis + elapsed_ms // 100
            self.render(self.current_decis)
            dist = self.next_step_target - self.current_decis
            if 0 < dist <= 3:
                self.on_step_warning()
            if self.current_decis >= self.next_step_target:
                self._advance_step()

    def _advance_step(self):
        new_target = next_step_decis(self.current_decis, self._is_fstop, self._precision_value)
        self.next_step_target = new_target
        if self._modality_step:
            self.relay_off()
            self.mode = MODE_NONE
        else:
            self._step_start_ticks = time.ticks_ms()
            self._step_base_decis = self.current_decis
        self.on_step(new_target)
```

- [ ] **Step 2: Verifica logica su host** — creare `tests/test_engine.py` con un clock e callback finti.

```python
import sys
import types


def _install_fake_time(t):
    fake = types.ModuleType("time")
    fake.ticks_ms = lambda: t[0]
    fake.ticks_add = lambda a, b: a + b
    fake.ticks_diff = lambda a, b: a - b
    sys.modules["time"] = fake


def test_focus_counts_up():
    t = [0]
    _install_fake_time(t)
    import importlib
    import r080.engine as eng
    importlib.reload(eng)
    rendered = []
    e = eng.TimingEngine(
        render=lambda d: rendered.append(d),
        on_metronome=lambda: None, on_countdown_end=lambda: None,
        on_step=lambda n: None, on_step_warning=lambda: None, relay_off=lambda: None,
    )
    e.start_focus()
    e._tick_100ms(); e._tick_100ms()
    assert rendered == [1, 2]


def test_countdown_ends_and_calls_relay_off():
    t = [0]
    _install_fake_time(t)
    import importlib
    import r080.engine as eng
    importlib.reload(eng)
    calls = {"relay": 0, "end": 0}
    e = eng.TimingEngine(
        render=lambda d: None, on_metronome=lambda: None,
        on_countdown_end=lambda: calls.__setitem__("end", calls["end"] + 1),
        on_step=lambda n: None, on_step_warning=lambda: None,
        relay_off=lambda: calls.__setitem__("relay", calls["relay"] + 1),
    )
    e.start_countdown(10, gate_enabled=False)   # 1.0 s
    t[0] = 1100
    e._tick_100ms()
    assert calls["relay"] == 1
    assert calls["end"] == 1
    assert e.mode == eng.MODE_NONE
```

> NOTA: `machine.Timer` viene dallo stub; con `gate_enabled=False` non si arma, mantenendo il test puro.

Run: `python -m pytest tests/test_engine.py -v`
Expected: 2 PASSED.

- [ ] **Step 3: Commit**

```bash
git add r080/engine.py tests/test_engine.py
git commit -m "feat(engine): asyncio timing engine replacing Timer1 ISR"
```

---

## FASE 5 — Presentazione, input, orchestrazione

### Task 5.1: `presentation.py` — rendering LCD + 7-seg

**Files:**
- Create: `r080/presentation.py`
- Riferimento parità: `include/functions/presentation.h` (layout righe 16x2, helper show*)

- [ ] **Step 1: Scrivere `r080/presentation.py`**

```python
from r080 import config

CHANNELS_STR = ("Ch:A", "Ch:B", "Ch:C")


class Presentation:
    """Rendering coordinato di LCD (16x2) e 7-seg. Layout parità con l'originale."""

    def __init__(self, lcd, seg7, state, op_status):
        self.lcd = lcd
        self.seg7 = seg7
        self.state = state
        self.op_status = op_status

    # --- 7 segmenti ---
    def show_time(self, decis):
        self.seg7.show_time(decis)

    # --- LCD ---
    def _write_at(self, col, row, text):
        self.lcd.set_cursor(col, row)
        self.lcd.send_string(text)

    def show_welcome(self):
        self.lcd.clear()
        self._write_at(0, 0, "R080 TIMER")
        self._write_at(0, 1, "PRONTO")

    def render_full(self):
        op = self.op_status.current
        self.lcd.clear()
        # Riga 0: nome modalità + tipo precisione (col 12) + modalità (col 14, solo test strip)
        self._write_at(0, 0, op.name[:11])
        if op.has_precision:
            self._write_at(12, 0, "F" if op.precision_type == config.FSTOP else "S")
        if op.has_modality:
            self._write_at(14, 0, "PP" if op.modality == config.STEP_BY_STEP else "CO")
        self._render_row1()
        self.show_time(self.state.time_set)

    def _render_row1(self):
        op = self.op_status.current
        if op.has_precision:
            self._write_at(0, 1, ("Pr:%s" % op.precision_label())[:8])
        # offset F-stop ai col 5..7
        if op.has_precision and op.precision_type == config.FSTOP:
            self._write_at(5, 1, self.state.fstop.format_offset())
        # canale col 9
        self._write_at(9, 1, CHANNELS_STR[self.state.channel - 1])
        # ♪ col 14 (slot custom 1) se non muto
        self.lcd.set_cursor(14, 1)
        self.lcd.write_char(1 if not self.state.mute else ord(" "))
        # ⚡ col 15 (slot custom 2) se relay attivo sul canale
        self.lcd.set_cursor(15, 1)
        self.lcd.write_char(2 if self.state.channels_relay_activated[self.state.channel - 1] else ord(" "))
```

- [ ] **Step 2: Commit**

```bash
git add r080/presentation.py
git commit -m "feat(presentation): LCD + 7-seg rendering"
```

### Task 5.2: `inputs.py` — gestione keypad/encoder/pulsanti

**Files:**
- Create: `r080/inputs.py`
- Riferimento parità: `keypad_mgmt.h` (evaluateKey, switchToChannel, nav F-stop), `encoder_mgmt.h`

- [ ] **Step 1: Scrivere `r080/inputs.py`**

```python
from r080 import config
from r080.hardware.keypad import PRESSED, HOLD, RELEASED


class InputManager:
    """Gestisce input quando il timer e' fermo. Muta state e op_status, salva su NVS."""

    def __init__(self, state, op_status, keypad, enc_mode, enc_prec,
                 enc_mode_btn, enc_prec_btn, presentation, sound, seg7, storage):
        self.state = state
        self.op = op_status
        self.keypad = keypad
        self.enc_mode = enc_mode
        self.enc_prec = enc_prec
        self.enc_mode_btn = enc_mode_btn
        self.enc_prec_btn = enc_prec_btn
        self.pres = presentation
        self.sound = sound
        self.seg7 = seg7
        self.storage = storage
        self._last_mode_pos = None
        self._last_prec_pos = None
        # callback dei pulsanti encoder
        self.enc_mode_btn.on_click = self._on_mode_button
        self.enc_prec_btn.on_click = self._on_prec_button

    # --- encoder ---
    def poll_encoders(self):
        mp = self.enc_mode.position() >> 2
        if mp != self._last_mode_pos:
            self._last_mode_pos = mp
            self.op.select_by_encoder(mp)
            self.pres.render_full()
        pp = self.enc_prec.position() >> 2
        if pp != self._last_prec_pos:
            self._last_prec_pos = pp
            if self.op.current.has_precision:
                self.op.set_precision_by_encoder(pp)
                self._recompute_fstop_if_needed()
                self.pres.render_full()
        self.enc_mode_btn.update()
        self.enc_prec_btn.update()

    def _recompute_fstop_if_needed(self):
        op = self.op.current
        if op.precision_type == config.FSTOP and self.state.fstop.base_time > 0:
            self.state.time_set = self.state.fstop.get_calculated_time(op.precision_value())

    def _on_mode_button(self):
        self.op.current.toggle_modality()
        self.pres.render_full()

    def _on_prec_button(self):
        op = self.op.current
        op.toggle_precision_type()
        if op.precision_type == config.FSTOP:
            self.state.fstop.set_base(self.state.time_set if self.state.time_set > 0 else 1)
        self.pres.render_full()

    # --- keypad ---
    def poll_keypad(self):
        key, ev = self.keypad.poll()
        if key is None:
            return
        if ev == HOLD:
            self._on_hold(key)
        elif ev == RELEASED:
            self._on_released(key)

    def _on_hold(self, key):
        if key == "*":
            self.state.mute = not self.state.mute
            if not self.state.mute:
                self.sound.emit(config.TONE_UP, 200)
            self._save_and_render()
        elif key == "#":
            i = self.state.channel - 1
            self.state.channels_relay_activated[i] = not self.state.channels_relay_activated[i]
            self._save_and_render()

    def _on_released(self, key):
        op = self.op.current
        if key in ("A", "B", "C"):
            self._switch_channel({"A": 1, "B": 2, "C": 3}[key])
        elif key == "D":
            self._reset_channel()
        elif key == "*":
            self._decrement(op)
        elif key == "#":
            self._increment(op)
        elif key.isdigit() and op.name != "FOCUS":
            self._digit(int(key))
        self._save_and_render()

    def _decrement(self, op):
        if op.name == "FOCUS":
            self.state.brightness = max(1, self.state.brightness - 1)
            self.seg7.set_brightness(self.state.brightness)
        elif op.precision_type == config.SECONDS:
            self.state.time_set = max(1, self.state.time_set - op.precision_value() * 10)
        else:  # FSTOP
            self.state.fstop.step_down()
            self.state.time_set = self.state.fstop.get_calculated_time(op.precision_value())

    def _increment(self, op):
        if op.name == "FOCUS":
            self.state.brightness = min(10, self.state.brightness + 1)
            self.seg7.set_brightness(self.state.brightness)
        elif op.precision_type == config.SECONDS:
            self.state.time_set = min(9999, self.state.time_set + op.precision_value() * 10)
        else:  # FSTOP
            self.state.fstop.step_up()
            self.state.time_set = self.state.fstop.get_calculated_time(op.precision_value())

    def _digit(self, d):
        if self.state.n_cifra == 0:
            self.state.time_set = d
        else:
            self.state.time_set = min(9999, self.state.time_set * 10 + d)
        self.state.n_cifra = (self.state.n_cifra + 1) % 4
        self.state.appo_time_set = self.state.time_set
        self.state.fstop.set_base(self.state.time_set)

    def _switch_channel(self, ch):
        self.state.channel_time[self.state.channel - 1] = self.state.time_set
        self.state.channel = ch
        self.state.time_set = self.state.channel_time[ch - 1]
        self.state.appo_time_set = self.state.time_set
        self.state.n_cifra = 0
        self.state.fstop.set_base(self.state.time_set)

    def _reset_channel(self):
        self.state.channel_time[self.state.channel - 1] = 0
        self.state.fstop.reset()
        self.state.time_set = 0
        self.state.appo_time_set = 0

    def _save_and_render(self):
        self.storage.save(self.state)
        self.pres.render_full()
```

- [ ] **Step 2: Commit**

```bash
git add r080/inputs.py
git commit -m "feat(inputs): keypad/encoder/button handling"
```

### Task 5.3: `app.py` + `main.py` + `boot.py` — orchestrazione asyncio

**Files:**
- Create: `r080/app.py`, `main.py`, `boot.py`
- Riferimento parità: `src/main.cpp` (setup/loop), `running_mgmt.h` (manageMainButtonAction/onStop/manageStarted)

- [ ] **Step 1: Scrivere `boot.py`**

```python
import gc
import machine

machine.freq(240_000_000)
gc.enable()
```

- [ ] **Step 2: Scrivere `main.py`**

```python
import uasyncio as asyncio
from r080.app import App

App().run()
```

- [ ] **Step 3: Scrivere `r080/app.py`**

```python
import time
import uasyncio as asyncio
from machine import Pin, UART
from r080 import config
from r080.state import AppState
from r080.operations import build_operations, OperationStatus
from r080.storage import Storage
from r080.sound import SoundPlayer
from r080.engine import TimingEngine
from r080.presentation import Presentation
from r080.inputs import InputManager
from r080.hardware.relay import Relay
from r080.hardware.buzzer import Buzzer
from r080.hardware.seg7 import Seg7
from r080.hardware.lcd1602_rgb import LCD1602RGB
from r080.hardware.keypad import Keypad
from r080.hardware.rotary import Rotary
from r080.hardware.button import Button


class App:
    def __init__(self):
        self.state = AppState()
        self.op = OperationStatus(build_operations())
        # hardware
        self.relay = Relay()
        self.buzzer = Buzzer()
        self.seg7 = Seg7(UART(2, baudrate=9600, tx=config.SEG7_TX))
        self.lcd = LCD1602RGB()
        self.keypad = Keypad()
        self.enc_mode = Rotary(config.ENC_MODE_CLK, config.ENC_MODE_DT)
        self.enc_prec = Rotary(config.ENC_PREC_CLK, config.ENC_PREC_DT)
        self.enc_mode_btn = Button(config.ENC_MODE_SW)
        self.enc_prec_btn = Button(config.ENC_PREC_SW)
        self.main_btn = Button(config.MAIN_BTN_PIN)
        self.storage = Storage()
        # software
        self.sound = SoundPlayer(self.buzzer, muted=lambda: self.state.mute)
        self.pres = Presentation(self.lcd, self.seg7, self.state, self.op)
        self.engine = TimingEngine(
            render=self.pres.show_time,
            on_metronome=lambda: self.sound.emit(config.TONE_DOWN, 80),
            on_countdown_end=self._on_countdown_end,
            on_step=self._on_step,
            on_step_warning=lambda: self.sound.emit(config.TONE_UP, 50),
            relay_off=self.relay.off,
        )
        self.inputs = InputManager(
            self.state, self.op, self.keypad, self.enc_mode, self.enc_prec,
            self.enc_mode_btn, self.enc_prec_btn, self.pres, self.sound,
            self.seg7, self.storage,
        )
        self.main_btn.on_click = self._toggle_main

    def _toggle_main(self):
        self.state.is_main_btn_pressed = not self.state.is_main_btn_pressed

    def _on_countdown_end(self):
        self.state.is_action_started = False
        self.state.is_main_btn_pressed = False
        self.sound.emit_stop()
        self.state.time_set = self.state.appo_time_set   # repeat-last-exposure

    def _on_step(self, new_target):
        self.sound.emit(config.TONE_UP, 150)
        op = self.op.current
        if op.modality == config.STEP_BY_STEP:
            self.state.is_action_started = False
            self.state.is_main_btn_pressed = False
            self.pres.show_time(new_target)

    def _start_action(self):
        op = self.op.current
        gate = self.state.channels_relay_activated[self.state.channel - 1]
        self.relay.set_gate(gate)
        self.state.is_action_started = True
        if op.name == "FOCUS":
            self.engine.start_focus()
        elif op.name == "COUNTDOWN":
            self.engine.start_countdown(self.state.appo_time_set, gate)
        else:  # TEST STRIP
            resume = (op.modality == config.STEP_BY_STEP and self.engine.current_decis > 0)
            self.engine.start_test_strip(
                self.state.appo_time_set,
                op.precision_type == config.FSTOP,
                op.precision_value(),
                op.modality == config.STEP_BY_STEP,
                resume,
            )

    def _stop_action(self):
        self.engine.stop()
        self.relay.off()
        self.state.is_action_started = False
        self.sound.emit_stop()
        self.pres.show_time(self.state.time_set)

    def setup(self):
        self.relay.off()
        self.lcd.init()
        self.lcd.set_rgb(255, 0, 0)               # rosso fisso
        self.lcd.custom_symbol(1, config.NOTE_GLYPH)
        self.lcd.custom_symbol(2, config.SPINA_GLYPH)
        self.seg7.reset()
        self.pres.show_welcome()
        time.sleep(3)                              # splash (come delay(3000))
        if self.storage.load(self.state):
            self.seg7.set_brightness(self.state.brightness)
        self.pres.render_full()
        print("=== R080 TIMER PRONTO ===")

    async def _input_loop(self):
        while True:
            self.main_btn.update()
            if self.state.is_main_btn_pressed and not self.state.is_action_started:
                self._start_action()
            elif (not self.state.is_main_btn_pressed) and self.state.is_action_started:
                self._stop_action()
            if not self.state.is_action_started:
                self.inputs.poll_encoders()
                self.inputs.poll_keypad()
            self.sound.tick()
            await asyncio.sleep_ms(5)

    async def _main(self):
        self.setup()
        await asyncio.gather(self._input_loop(), self.engine.run_render())

    def run(self):
        asyncio.run(self._main())
```

- [ ] **Step 4: Verificare l'intera suite host**

Run: `python -m pytest -q`
Expected: tutti i test PASSED (config, fstop, stepmath, state, operations, storage, seg7, keypad, sound, engine).

- [ ] **Step 5: Commit**

```bash
git add r080/app.py main.py boot.py
git commit -m "feat(app): asyncio orchestration and entrypoint"
```

---

## FASE 6 — Toolchain, deploy, verifica on-device

### Task 6.1: Script di flash/deploy e Makefile

**Files:**
- Create: `tools/flash.sh`, `tools/deploy.sh`, `Makefile`

- [ ] **Step 1: Scrivere `tools/flash.sh`**

```bash
#!/usr/bin/env bash
# Flash del firmware MicroPython sull'ESP32. Uso: tools/flash.sh <port> <firmware.bin>
set -euo pipefail
PORT="${1:?porta seriale, es. /dev/ttyUSB0}"
FW="${2:?path al firmware MicroPython .bin}"
esptool.py --chip esp32 --port "$PORT" erase_flash
esptool.py --chip esp32 --port "$PORT" --baud 460800 write_flash -z 0x1000 "$FW"
```

- [ ] **Step 2: Scrivere `tools/deploy.sh`**

```bash
#!/usr/bin/env bash
# Carica i file .py sull'ESP32 via mpremote. Uso: tools/deploy.sh <port>
set -euo pipefail
PORT="${1:?porta seriale, es. /dev/ttyUSB0}"
mpremote connect "$PORT" fs cp boot.py :boot.py
mpremote connect "$PORT" fs cp main.py :main.py
mpremote connect "$PORT" fs cp -r r080 :
mpremote connect "$PORT" reset
```

- [ ] **Step 3: Scrivere `Makefile`**

```make
PORT ?= /dev/ttyUSB0
FW ?= micropython-esp32.bin

.PHONY: test flash deploy repl clean
test:
	python -m pytest -q
flash:
	bash tools/flash.sh $(PORT) $(FW)
deploy:
	bash tools/deploy.sh $(PORT)
repl:
	mpremote connect $(PORT) repl
clean:
	rm -rf .pytest_cache **/__pycache__
```

- [ ] **Step 4: Rendere eseguibili gli script e commit**

```bash
chmod +x tools/flash.sh tools/deploy.sh
git add tools Makefile
git commit -m "chore: flash/deploy tooling and Makefile"
```

### Task 6.2: README e CLAUDE.md aggiornati

**Files:**
- Create: `README.md` (sostituisce quello Arduino), `CLAUDE.md` (riscritto)

- [ ] **Step 1: Scrivere `README.md`** con: requisiti (firmware MicroPython ESP32, `esptool`, `mpremote`, `pytest`), mappa pin (tabella sopra), nota sui **pull-up esterni 10kΩ** sulle colonne keypad (GPIO 34–39), nota livelli 3.3 V (relay 3.3 V-trigger / level shifter per 7-seg/LCD se necessario), comandi `make test|flash|deploy|repl`.

- [ ] **Step 2: Scrivere `CLAUDE.md`** per il nuovo stack: progetto MicroPython (NON Arduino/PlatformIO), struttura `r080/`, modello asyncio, persistenza NVS, unità decisecondi, indicizzazione canali 1-based in stato / 0-based negli array, comandi di test/deploy, parità funzionale con DOCS.

- [ ] **Step 3: Commit**

```bash
git add README.md CLAUDE.md
git commit -m "docs: README and CLAUDE.md for MicroPython/ESP32"
```

### Task 6.3: Verifica end-to-end su hardware

> Richiede ESP32 collegato + componenti cablati secondo la mappa pin + pull-up esterni keypad.

- [ ] **Step 1: Flash firmware + deploy**

```bash
make flash PORT=/dev/ttyUSB0 FW=micropython-esp32.bin
make deploy PORT=/dev/ttyUSB0
make repl PORT=/dev/ttyUSB0
```
Expected: al reset compare sul seriale `=== R080 TIMER PRONTO ===`; LCD rosso con welcome poi schermata operativa.

- [ ] **Step 2: Test funzionali manuali (parità con `DOCS.md`)**

- [ ] FOCUS: pressione pulsante → relay ON e conteggio crescente sul 7-seg; `*`/`#` regolano brightness.
- [ ] COUNTDOWN: digita tempo (decis) da keypad; start → metronomo ogni secondo; a fine corsa relay OFF preciso, sequenza stop, `time_set` torna al valore impostato (repeat-last-exposure).
- [ ] TEST STRIP: step in SECONDS e FSTOP; beep di warning e di step; toggle CONTINUO/PASSO-PASSO col pulsante encoder mode.
- [ ] F-Stop: `*`/`#` cambiano offset; tempo ricalcolato e clampato 1..9999; LCD mostra `"+02"/" 00"`.
- [ ] Canali A/B/C: memoria tempo per canale; `D` reset; hold-`#` gate relay (⚡); hold-`*` mute (♪).
- [ ] Persistenza: spegni/riaccendi → impostazioni ripristinate da NVS.

- [ ] **Step 3: Verifica timing**

Cronometrare un countdown di 10.0 s: relay deve aprirsi entro ~±50 ms dal previsto.

- [ ] **Step 4: Commit finale eventuali fix**

```bash
git add -A
git commit -m "test: on-device verification fixes"
```

---

## Self-Review (eseguita)

- **Copertura spec:** tutte le sezioni dello spec hanno un task — pin map (header+config 1.1), uasyncio engine (4.2), NVS (2.1), driver LCD/7-seg/keypad/encoder/buzzer/relay (3.x), F-stop (1.2), test-strip math (1.3), operations (1.5), presentation (5.1), inputs (5.2), app/setup order (5.3), toolchain/git (0.1, 6.1), README/CLAUDE (6.2), verifica on-device (6.3).
- **Placeholder:** nessun TBD/TODO; codice completo in ogni step di codice; README/CLAUDE (6.2) descritti per contenuto (documenti, non codice testabile) — accettabile.
- **Coerenza tipi:** nomi coerenti tra task — `AppState.channel_index()`, `Operation.precision_value()/precision_type/modality`, `OperationStatus.select_by_encoder/set_precision_by_encoder`, `FStopState.get_calculated_time/step_up/step_down/format_offset/set_base/reset`, `Storage.load/save`, `SoundPlayer.emit/emit_stop/tick`, `TimingEngine.start_focus/start_countdown/start_test_strip/stop/run_render`, `Seg7.show_time/reset/set_brightness`, `Keypad.poll`, `Rotary.position`, `Button.update/on_click`. Verificati gli usi incrociati in `inputs.py`/`app.py`.
- **Rischi noti documentati:** pull-up esterni keypad, livelli 3.3 V, conflitti di rebase su main.
