# Spec di design — Port R080 Timer → MicroPython su ESP32

Data: 2026-06-13
Stato: approvato in brainstorming, in attesa di revisione spec

## 1. Contesto e obiettivo

Il firmware attuale (`R080Timer_MEGA`) è un timer per ingranditore da camera oscura scritto in
C++/Arduino per Arduino Mega 2560, costruito come **singola translation unit** (header che
*definiscono* funzioni, inclusi in ordine da `src/main.cpp`), con motore temporale a ISR Timer1
1 kHz, persistenza EEPROM e driver hardware dedicati.

L'obiettivo è riscrivere lo **stesso software, con identica funzionalità**, in **MicroPython** su
**ESP32 NodeMCU (WROOM-32 DevKit 38-pin)**, usando gli **stessi componenti hardware**. Non è una
traduzione meccanica: l'ESP32 ha meno GPIO (alcuni solo-input o strapping), niente EEPROM, e
MicroPython non gira sotto PlatformIO. Tre sottosistemi vanno *ripensati*:

1. **Timing/concorrenza** → da ISR Timer1 a `uasyncio` (cooperativo, non-bloccante), con un
   one-shot `machine.Timer` per garantire la precisione dello spegnimento relay a fine countdown.
2. **Persistenza** → da EEPROM `eeprom_update_*` a `esp32.NVS` (key-value in flash, wear-leveled).
3. **Toolchain** → da PlatformIO a flusso MicroPython standard (`esptool` per il firmware,
   `mpremote` per i file `.py`).

Risultato atteso: parità funzionale 1:1 — 3 modalità (Focus, Countdown, Test Strip), navigazione
F-Stop, 3 canali con memoria, persistenza impostazioni, suoni non-bloccanti, layout LCD e 7-seg
identici, unità di tempo in **decisecondi**.

## 2. Decisioni di design (confermate)

| Aspetto      | Decisione                                                                 |
|--------------|---------------------------------------------------------------------------|
| Scheda       | ESP32-WROOM-32 DevKit 38-pin (mappa pin proposta sotto)                    |
| Timing       | `uasyncio` + one-shot `machine.Timer` per relay-off preciso               |
| Persistenza  | `esp32.NVS` (mirror dei 13 campi EEPROM, write-on-change)                 |
| Toolchain    | `esptool` (flash firmware) + `mpremote` (deploy `.py`), niente PlatformIO |
| Git          | nuovo branch dal branch corrente, contenuto come repo separato, rebase su `main` in seguito |

## 3. Strategia Git / Toolchain

- **Branch**: creare `feature/micropython-esp32-port` **a partire dal branch corrente**
  (`feature/rewrite_to_improve_stability_and_performance`). Su questo branch si sostituisce tutto
  il contenuto come se fosse un repo separato MicroPython. Rebase su `main` in un secondo momento
  (atteso conflitto totale, da risolvere mantenendo il nuovo albero MicroPython).
- **Rimuovere** dal nuovo branch i file specifici Arduino: `src/`, `include/`, `lib/`,
  `platformio.ini`, `.pio/`.
- **Conservare/adattare**: `LICENSE`, `README*.md`, `DOCS*.md` (aggiornati per ESP32/MicroPython),
  `CLAUDE.md` (riscritto per il nuovo stack).
- **Toolchain**: nessun PlatformIO. Si flasha una volta il firmware MicroPython
  (`esptool.py write_flash`), poi si caricano i `.py` con `mpremote`. Uno script in `tools/`
  automatizza flash e deploy.

## 4. Mappa Pin proposta (ESP32-WROOM-32 38-pin)

Vincoli rispettati: GPIO 6–11 (flash) esclusi; GPIO 1/3 (REPL UART0) esclusi; strapping
(0,2,12,15) evitati per ingressi con pull-up; GPIO 34–39 sono **solo-input senza pull-up interno**.

| Funzione                     | GPIO         | Note                                            |
|------------------------------|--------------|-------------------------------------------------|
| I2C SDA (LCD 0x3E + RGB 0x60)| 21           | I2C0 default                                    |
| I2C SCL                      | 22           | I2C0 default                                    |
| 7-seg TX (UART2)             | 17           | `UART(2, tx=17)`, 9600 baud, TX-only            |
| Relay ingranditore           | 23           | output, LOW a boot                              |
| Buzzer                       | 19           | LEDC PWM (`machine.PWM`)                         |
| Pulsante principale          | 5            | input pull-up                                   |
| Encoder Mode CLK / DT / SW   | 16 / 4 / 18  | CLK su IRQ, tutti input pull-up                 |
| Encoder Precision CLK/DT/SW  | 25 / 26 / 27 | CLK su IRQ, tutti input pull-up                 |
| Keypad righe 0..3 (output)   | 13,14,32,33  | pilotate a scansione                            |
| Keypad colonne 0..3 (input)  | 34,35,36,39  | **solo-input → richiedono pull-up esterni 10kΩ**|

Nota hardware (fuori software): l'ESP32 è 3.3 V. Relay opto-isolato, LCD/RGB Waveshare e 7-seg
seriale vanno verificati a 3.3 V; le 4 colonne keypad richiedono resistori pull-up esterni perché
i GPIO 34–39 non hanno pull-up interni. Da documentare in `README`.

## 5. Struttura progetto (file da creare)

```
boot.py                  # setup minimale a boot (freq CPU, no auto-connect WiFi)
main.py                  # entrypoint: import app; uasyncio.run(app.run())
r080/
  config.py              # pin, toni (TONE_UP=600/TONE_DOWN=300), testi modalità, keymap, indirizzi I2C
  state.py               # stato globale: timeSet (decis), appoTimeSet, channel, channelTime[3],
                         #   channelsRelayActivated[3], nCifra, brightness, mute, fstop
  fstop.py               # FStopState: base/offset, get_calculated_time, step_up/down, format_offset
  storage.py             # NVS load/save (mirror dei 13 campi EEPROM, write-on-change)
  hardware/
    lcd1602_rgb.py       # port driver Waveshare (machine.I2C): init, set_rgb, custom_symbol,
                         #   set_cursor, write_char, send_string
    seg7.py              # 7-seg seriale (machine.UART): reset 0x76, DP 0x77+maschera, brightness 0x7A, "%04d"
    keypad.py            # scanner 4x4 con debounce 50ms + stati HOLD/PRESSED/RELEASED (quirk incluso)
    rotary.py            # decoder quadratura IRQ-based (pos>>2) + classe pulsante encoder
    button.py            # OneButton-like: debounce 50ms, click 100ms, press 1000ms
    buzzer.py            # tone(freq, dur) via LEDC, no-op se mute
    relay.py             # gate relay (rispetta channelsRelayActivated[channel-1])
  engine.py              # motore temporale uasyncio (sostituisce Timer1 ISR): clock ticks_ms,
                         #   task countdown/teststrip/focus, render 100ms, metronomo, one-shot Timer relay-off
  sound.py               # SoundPlayer non-bloccante (sequenza stop, beep singoli) come task async
  operations.py          # modello Operation/OperationStatus + le 3 modalità e precision arrays
  presentation.py        # rendering LCD (righe 16x2) + 7-seg; helper show*()
  inputs.py              # gestione keypad/encoder/pulsanti → muta state, avvia/ferma engine
  app.py                 # orchestrazione: init hardware, loadSettings, task loop asyncio
tools/
  flash.sh               # esptool: erase + write_flash firmware MicroPython
  deploy.sh              # mpremote: upload ricorsivo dei .py + reset
Makefile                 # target: flash, deploy, repl, clean
README.md / DOCS.md      # adattati a ESP32/MicroPython
CLAUDE.md                # riscritto per il nuovo stack
```

## 6. Dettagli di implementazione chiave

### 6.1 Motore temporale (`engine.py`) — sostituisce l'ISR Timer1
- Clock: `time.ticks_ms()` / `ticks_diff` come fonte monotonica (al posto di `g_tickMs`).
- Stato modalità (`mode`: none/focus/countdown/teststrip) come nell'originale `g_modeActive`.
- **Render**: coroutine a 100 ms (sostituisce `g_tickFlag100ms` → `onTick100ms()`).
- **COUNTDOWN**: calcola `end = ticks_ms() + appoTimeSet*100`; render decis ogni 100 ms; metronomo
  `TONE_DOWN 80ms` a ogni secondo; a fine corsa → relay OFF + sequenza stop + `timeSet = appoTimeSet`
  (repeat-last-exposure). Per precisione spegnimento relay: one-shot `machine.Timer` armato a
  `start` che chiama `relay.off()` esattamente a fine tempo (equivalente al RELAY_OFF in ISR).
- **TEST_STRIP**: conta in su; al raggiungimento `next_step_target` → `TONE_UP 150ms`, ricalcolo
  target (FSTOP: `round(decis * 2**(1/prec))`, ≥+1; SECONDS: `decis + prec*10`), clamp 1..9999;
  beep di warning `TONE_UP 50ms` quando mancano 1..3 decis; in STEP_BY_STEP mette in pausa
  (relay OFF, mode=none), in CONTINOUS prosegue.
- **FOCUS**: relay ON, conta in su, render 100 ms.
- Atomicità: con asyncio cooperativo non servono `ATOMIC_BLOCK`; gli aggiornamenti di stato
  avvengono tra `await`. L'unico contesto IRQ reale è il callback del one-shot Timer del relay →
  mantenere leggero (solo `relay.off()` + set flag).

### 6.2 F-Stop (`fstop.py`)
Replica esatta: `calculated = base * 2**(offset/precisionValue)`, `round(x)`, clamp `1..9999`;
short-circuit se base≤0 o prec≤0; `format_offset` → `"+02"/"-01"/" 00"`. Riusa la stessa logica dei
punti di set di `baseTime` (ogni cifra keypad, switch canale, switch SECONDS→FSTOP, lazy-init su `*`/`#`).

### 6.3 Persistenza (`storage.py`) — NVS
Namespace `"r080"`. Chiavi `i32`: `version`(=1, sostituisce magic 0xA5), `brightness`, `mute`,
`channel`, `ch0/1/2_time`, `ch0/1/2_relay`. `load()` → se `version` assente/≠1 ritorna False (primo
boot, default). Mirror della semantica `eeprom_update`: leggere prima e scrivere solo se cambiato,
poi `commit()`. Clamp identici (brightness 1..10, channel 1..3, tempi 0..9999). `timeSet =
channelTime[channel-1]; appoTimeSet = timeSet`.

### 6.4 Driver LCD (`hardware/lcd1602_rgb.py`)
Port 1:1 del Waveshare: `machine.I2C(0, scl=22, sda=21)`. Comandi a 0x3E con prefisso `0x80`
(comando) / `0x40` (dato), sequenza init documentata (function set ×3, display control, clear,
entry mode), registri RGB a 0x60 (MODE1=0x00, OUTPUT=0xFF, MODE2=0x20, PWM su REG_RED/GREEN/BLUE).
Backlight rosso fisso `set_rgb(255,0,0)`. Custom symbol via CGRAM (note ♪ slot 1, spina ⚡ slot 2).

### 6.5 7-seg (`hardware/seg7.py`)
`UART(2, baudrate=9600, tx=17)`. Identici: `0x76` reset, `0x77`+`0b00000100` DP a cifra 1,
`0x7A`+brightness, e `"%04d" % value` per le cifre. `show_time(100)` → `10.0`.

### 6.6 Input (`hardware/keypad.py`, `rotary.py`, `button.py`, `inputs.py`)
- Keypad: scanner a scansione righe, debounce 50 ms, macchina a stati per riprodurre
  HOLD/PRESSED/RELEASED e il **quirk** "HOLD non fa break" (HOLD su `*`=mute, `#`=relay canale;
  RELEASED gestisce A/B/C/D, `*`/`#`, cifre 0..9). Stessa keymap 4×4.
- Encoder: decoder quadratura via IRQ su CLK leggendo DT, `pos >> 2`, indice `(pos%N+N)%N`
  (N=3 modalità / N=11 precisione). Pulsanti encoder → toggle modality / precision type.
- Pulsante principale: classe stile OneButton (debounce 50/click 100/press 1000), click → toggle
  `is_main_btn_pressed`.

### 6.7 Modello modalità (`operations.py`)
In Python si modella con oggetti/closure invece dei 4 puntatori a funzione C++. `Operation` con
`show_presentation`, `switch_modality`, `switch_precision_type`, `started_action` + stato
(modality/precision type + indice precisione + nome). Tre costruttori (FOCUS senza prec/mod;
COUNTDOWN con prec SECONDS default; TEST_STRIP con mod CONTINOUS + prec SECONDS). Array
`FSTOP_PRECISIONS` {1,2,3,4,6,8,12,16,24,32,64} e `SECONDS_PRECISIONS` {1,2,3,4,5,6,7,8,10,15,20}.

### 6.8 App (`app.py` + `main.py`)
Ordine setup come l'originale: init pin/I2C/UART → config keypad/LCD/7seg → splash + `asyncio.sleep(3)`
→ `storage.load()` (riapplica brightness) → init operations → render iniziale → avvio task asyncio.
Loop asyncio: task input (poll ~5–10 ms), task render 100 ms, task suono, task engine modalità.
L'input gira solo a timer fermo (come `manageMainButtonAction`/`onStop` vs `manageStarted`).

## 7. Verifica end-to-end

Niente unit test su device (come l'originale). Verifica su hardware reale:

1. **Flash firmware**: `tools/flash.sh` → `esptool.py --chip esp32 write_flash 0x1000 <micropython.bin>`.
2. **Deploy**: `tools/deploy.sh` → `mpremote connect <port> fs cp -r r080/ main.py boot.py :` + reset.
3. **REPL/seriale**: `mpremote repl`. Verificare banner di boot equivalente a `=== R080 TIMER PRONTO ===`.
4. **Test funzionali manuali** (parità con DOCS):
   - FOCUS: relay ON, conteggio crescente, `*`/`#` regolano brightness 7-seg.
   - COUNTDOWN: inserimento tempo da keypad (decis), metronomo ogni secondo, auto-stop con relay
     OFF preciso, repeat-last-exposure (`timeSet` torna al valore impostato).
   - TEST STRIP: step in SECONDS e FSTOP, beep di warning e di step, CONTINUO vs PASSO-PASSO.
   - F-Stop: `*`/`#` cambiano offset, tempo ricalcolato e clampato 1..9999, display `"+02"/" 00"`.
   - Canali A/B/C: memoria tempo per canale, `D` reset, gate relay con hold-`#`, mute con hold-`*`.
   - Persistenza: spegnere/riaccendere → impostazioni ripristinate da NVS.
5. **Timing**: verificare con cronometro che un countdown di 10.0 s spenga il relay entro ~±50 ms.

## 8. Note / rischi
- I GPIO 34–39 (colonne keypad) richiedono pull-up esterni: senza, la scansione legge rumore.
- `pow`/float disponibili in MicroPython ESP32 (`2**(offset/prec)` ok).
- Eventuale problema di livelli 3.3 V con LCD/7-seg/relay è hardware, non risolvibile via software:
  da segnalare nel README (eventuale level shifter / relay 3.3 V-trigger).
- Il rebase su `main` produrrà conflitti totali (sostituzione completa): risolvere mantenendo il
  nuovo albero MicroPython.
