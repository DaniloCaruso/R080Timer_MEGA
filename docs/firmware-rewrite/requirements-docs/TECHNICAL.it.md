# TECHNICAL — Riscrittura del firmware R080 Timer

- **Feature:** `firmware-rewrite`
- **Data:** 2026-10-18
- **Fonte delle decisioni:** sessione di grilling del 2026-10-18; `docs/firmware-rewrite/decisions.md`; `GLOSSARY.md`; requisiti in `EARS.it.md`.
- **Codice letto:** tutto il firmware al tag `pre-rewrite` (`src/main.cpp`, `include/*.h`, `include/functions/*.h`, `lib/WaveShare-Lib/*`, `platformio.ini`), i sorgenti delle librerie in `.pio/libdeps/megaatmega2560/` (OneButton 2.6.1, Keypad 3.1.1, Encoder 1.4.4), `Tone.cpp`, `HardwareSerial.h` e `Wire.h` del core `framework-arduino-avr` 5.2.0, il bootloader `bootloaders/stk500v2/stk500boot.c`, il disassemblato della ISR attuale.

## Panoramica

**Stato attuale.** Il firmware è un'unica unità di traduzione: `src/main.cpp` include in ordine fisso gli header `include/functions/*.h`, che definiscono funzioni e leggono e scrivono variabili globali di `include/variables.h`. Il motore temporale è già su Timer1 a 1 kHz (`include/functions/timer_isr.h`), ma il resto ha difetti strutturali verificati nel codice:
- `onStop()` (`include/functions/running_mgmt.h`) chiama a ogni giro di loop `stopAllTimers()`, che rilancia `emitStopSound()` e riscrive `showTime(timerCurrentValue)`: da fermo il beep di stop suona in loop e il 7 segmenti torna a 0000;
- `setOperationStatus()` (`include/functions/operations.h`) chiama `resetTime()` e azzera il tempo caricato da `loadSettings()`;
- la libreria `lib/WaveShare-Lib` esegue `delay(5)` per ogni byte I²C in `Waveshare_LCD1602_RGB::send()`, e `evaluateEncoderPrec()` ridisegna la precisione a ogni giro: il loop da fermo dura circa 110 ms e le pressioni brevi si perdono;
- la ISR calcola `g_remainingMs % 1000` su 32 bit e chiama `__udivmodsi4` a ogni millisecondo;
- nessuna logica è testabile senza la scheda.

**Obiettivo.** Firmware nuovo in tre strati: una libreria di dominio in C++ puro (`lib/r080_core/`, NUOVO) testata sul PC; driver Arduino sottili (`src/drivers/`, NUOVO) che implementano le interfacce del dominio; un `src/main.cpp` riscritto che li collega. Il codice al tag `pre-rewrite` resta solo come riferimento.

## Vincoli e invarianti

- **Niente blocchi nel loop.** Nessun `delay()` né attesa attiva dopo l'avvio; ogni driver lavora a passi brevi. Il loop chiama `wdt_reset()` a ogni giro.
- **ISR minima.** La ISR di Timer1 esegue solo somme, confronti e scritture su `PORTL`, senza divisioni né chiamate a funzioni non inline.
- **Variabili condivise.** Ogni lettura o scrittura dal main di stato condiviso con una ISR, se più larga di 8 bit, avviene in `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)` (regola già in `CLAUDE.md`).
- **EEPROM.** Si scrive solo con `eeprom_update_*`, mai con `eeprom_write_*` (regola già in `CLAUDE.md`).
- **RAM.** Niente `String`, niente `ArduinoSTL`, niente allocazione dinamica (`new`/`malloc`); i testi costanti stanno in `PROGMEM`. Le vtable delle interfacce virtuali su AVR occupano RAM: il numero di interfacce resta piccolo.
- **Unità.** Il tempo interno è in millisecondi `uint32_t`; il decimo di secondo è l'unità di interfaccia (REQ-11.5).
- **Dominio senza Arduino.** Nessun file in `lib/r080_core/` include `Arduino.h`, `avr/*` o librerie Arduino.
- **Invarianti superati.** Il modello "single translation unit" e l'ordine fisso degli include descritti in `CLAUDE.md` decadono e vanno riscritti (C-26).

## Modifiche

### C-1 — Repository: branch e tag
- **Soddisfa:** REQ-12.4
- **Stato attuale:** branch `feature/rewrite_to_improve_stability_and_performance` e `feature/micropython-esp32-port`; tag locale `pre-rewrite` su `c36256f`, non inviato al remote. `CLAUDE.md` (§2) e `docs/agents/issue-tracker.md` (tabella "Platform labels") citano i nomi vecchi.
- **Modifica:** su `main` si aggiornano i nomi in `CLAUDE.md` e `docs/agents/issue-tracker.md`; si creano e inviano `feature/arduino-mega-firmware` e `feature/esp32-micropython-firmware` dai branch attuali, ribasati su `main`; si invia il tag `pre-rewrite`; si cancellano i branch remoti vecchi solo dopo conferma esplicita dell'utente. Il `CLAUDE.md` del branch ESP32 si aggiorna su quel branch.
- **Comportamento atteso:** `git branch -r` mostra i due nomi nuovi; `git tag` mostra `pre-rewrite` anche sul remote.
- **Verifica:** `git ls-remote --heads origin` e `git ls-remote --tags origin`.

### C-2 — `platformio.ini`: ambienti Mega e native
- **Soddisfa:** REQ-10.3, REQ-11.3, REQ-11.5
- **Stato attuale:** un solo ambiente `[env:megaatmega2560]` con `lib_deps` `paulstoffregen/Encoder`, `chris--a/Keypad`, `marcoschwartz/LiquidCrystal_I2C`, `stutchbury/EncoderButton`, `mathertel/OneButton`.
- **Modifica:**
  ```ini
  [platformio]
  default_envs = megaatmega2560

  [env:megaatmega2560]
  platform = atmelavr
  board = megaatmega2560
  framework = arduino
  build_unflags = -std=gnu++11
  build_flags = -std=gnu++17 -Wall -Wextra -DFW_VERSION=\"2.0.0\"
  lib_deps =
    paulstoffregen/Encoder@^1.4.4
    chris--a/Keypad@^3.1.1
    mathertel/OneButton@^2.6.1

  [env:native]
  platform = native
  test_framework = unity
  build_flags = -std=gnu++17 -Wall -Wextra
  lib_ignore = Encoder, Keypad, OneButton
  ```
  Si rimuovono `LiquidCrystal_I2C` ed `EncoderButton`.
- **Comportamento atteso:** `pio run` compila per il Mega; `pio test -e native` compila `lib/r080_core` e i test con il compilatore del PC.
- **Verifica:** entrambi i comandi terminano con exit 0.

### C-3 — `.github/workflows/firmware.yml` (NUOVO): CI
- **Soddisfa:** REQ-11.4
- **Stato attuale:** nessuna CI.
- **Modifica:** workflow su `push` e `pull_request`: installa Python e PlatformIO, esegue `pio run -e megaatmega2560` e `pio test -e native`, con cache di `~/.platformio`.
- **Comportamento atteso:** ogni push mostra su GitHub lo stato di compilazione e test.
- **Verifica:** un push con un test volutamente rotto fa fallire il workflow.

### C-4 — Rimozione del firmware precedente
- **Soddisfa:** REQ-10.3, REQ-11.1
- **Stato attuale:** `include/types.h`, `include/constants.h`, `include/variables.h`, `include/functions/{actions,base,config,encoder_mgmt,fstop,keypad_mgmt,operations,presentation,running_mgmt,sound,storage,timer_isr,timer_manager}.h`, `lib/WaveShare-Lib/Waveshare_LCD1602_RGB.{h,cpp}`.
- **Modifica:** i file elencati vengono cancellati nello stesso commit in cui entra il nuovo `src/main.cpp` (C-24), così ogni commit compila. Restano consultabili al tag `pre-rewrite`.
- **Comportamento atteso:** nessun riferimento residuo a `operationStatus`, `timeSet`, `appoTimeSet`, `g_remainingMs` & co.
- **Verifica:** `grep -rn "timeSet\|operationStatus\|Waveshare_LCD1602_RGB" src lib include` non trova nulla; `pio run` ok.

### C-5 — `lib/r080_core/Hal.h`, `Time.h` (NUOVO): interfacce e unità
- **Soddisfa:** REQ-11.1, REQ-11.2, REQ-11.5
- **Stato attuale:** il codice chiama direttamente `lcd`, `sevenSegmentsDisplay`, `tone()`, `eeprom_*`, `RELAY_ON()`.
- **Modifica:** interfacce astratte usate dal dominio:
  ```cpp
  struct Lcd       { virtual bool writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) = 0;
                     virtual bool setBrightness(uint8_t level) = 0; virtual bool online() const = 0; };
  struct SevenSeg  { virtual void showDeciseconds(uint16_t ds) = 0; virtual void setBrightness(uint8_t level) = 0; };
  struct Buzzer    { virtual void play(uint16_t hz, uint16_t ms) = 0; virtual void stop() = 0; };
  struct Storage   { virtual void read(uint16_t addr, uint8_t* dst, uint16_t n) = 0;
                     virtual void update(uint16_t addr, const uint8_t* src, uint16_t n) = 0; };
  struct EventLog  { virtual void line(const char* text) = 0; };
  struct Clock     { virtual uint32_t nowMs() const = 0; };
  ```
  `Time.h`: `constexpr uint32_t MS_PER_DS = 100; constexpr uint16_t DS_MIN = 1, DS_MAX = 9999;` e funzioni `dsToMs()`, `msToDs()`. Il relè non ha un'interfaccia nel dominio: lo comandano solo `TimingCore` (C-6) tramite i bit di uscita e il driver (C-16).
- **Comportamento atteso:** il dominio compila sul PC senza header Arduino.
- **Verifica:** `pio test -e native` compila; `grep -rn "Arduino.h\|avr/" lib/r080_core` non trova nulla.

### C-6 — `lib/r080_core/TimingCore.{h,cpp}` (NUOVO): logica della ISR
- **Soddisfa:** REQ-1.1, REQ-1.2, REQ-1.3, REQ-1.5, REQ-1.6, REQ-3.3, REQ-3.4, REQ-3.11, REQ-3.12, REQ-3.14, REQ-8.3, REQ-11.3
- **Stato attuale:** la logica sta dentro `ISR(TIMER1_COMPA_vect)` in `include/functions/timer_isr.h`, con `g_modeActive` (0–3), `g_remainingMs`, `g_elapsedMs`, `g_nextStepTargetMs` e il modulo a 32 bit per il metronomo; il passo-passo spegne il relè dal main in `onTestStripStep()`.
- **Modifica:** classe pura chiamata una volta per tick dalla ISR:
  ```cpp
  enum class Run : uint8_t { Idle, Focus, Countdown, StripContinuous, StripStepped };
  enum : uint8_t { EV_END = 1, EV_STEP = 2, EV_METRONOME = 4, EV_PREWARN = 8,
                   EV_FOCUS_WARN = 16, EV_FOCUS_OFF = 32, EV_FRAME = 64 };
  struct TickOut { uint8_t events; bool relayOff; };

  class TimingCore {
   public:
    void startFocus(uint32_t limitMs);
    void startCountdown(uint32_t durationMs);
    void startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs);
    void setNextStep(uint32_t nextStepMs);
    void cancel();
    void setCorrection(int16_t ppm);           // delega a ClockCorrection (C-7)
    TickOut tick();                             // chiamata dalla ISR, 1 kHz
    uint32_t elapsedMs() const; uint32_t remainingMs() const; Run run() const;
   private:
    Run run_; uint32_t elapsed_, target_, limit_, next_; uint16_t msInSecond_; uint8_t frame_;
  };
  ```
  Regole di `tick()`: avanza `elapsed_` di 0, 1 o 2 ms secondo la Correzione del clock; `msInSecond_` (16 bit) conta fino a 1000 e alza `EV_METRONOME` solo in `Countdown`; `frame_` alza `EV_FRAME` ogni 100 ms per il refresh dei display; in `Countdown`, a `elapsed_ >= target_` → `relayOff`, `EV_END`, `Idle`; in Test strip, a `next_ - elapsed_` uguale a 300, 200, 100 ms → `EV_PREWARN`, a `elapsed_ >= next_` → `EV_STEP` e, se `StripStepped`, `relayOff` e pausa; a 999,9 s → `relayOff`, `EV_END`; in `Focus`, a `limit_ - 10000` → `EV_FOCUS_WARN`, a `limit_` → `relayOff`, `EV_FOCUS_OFF`. Il metronomo conta dall'inizio dell'Esposizione (REQ-1.6), non dal tempo residuo.
- **Comportamento atteso:** tutte le decisioni temporali sono prese al tick esatto, senza dipendere dal loop.
- **Verifica:** test native in `test/test_timing_core/` che chiamano `tick()` N volte e controllano eventi e `relayOff` al ms esatto (countdown 10,0 s → `relayOff` al tick 10000; metronomo ai tick 1000, 2000…; preavviso a −300/−200/−100; Focus a limite −10 s e al limite); disassemblato della ISR senza `call` (vedi C-16).

### C-7 — `lib/r080_core/ClockCorrection.{h,cpp}` (NUOVO): Correzione del clock
- **Soddisfa:** REQ-1.4, REQ-1.7, REQ-9.12
- **Stato attuale:** non esiste.
- **Modifica:** accumulatore in parti per milione eseguito a ogni tick:
  ```cpp
  // ritorna quanti ms contare in questo tick: 0, 1 o 2
  inline uint8_t ClockCorrection::step() {
    acc_ += ppm_;                                   // int32_t acc_, int16_t ppm_ in [-1000, 1000]
    if (acc_ >=  1000000L) { acc_ -= 1000000L; return 2; }
    if (acc_ <= -1000000L) { acc_ += 1000000L; return 0; }
    return 1;
  }
  ```
  `ppmFromMeasurement(uint32_t nominalMs, uint32_t measuredMs, int16_t currentPpm)` calcola la nuova correzione: `(1 + cur/1e6) · measured/nominal − 1`, in ppm, limitata a ±1000.
- **Comportamento atteso:** con +200 ppm il tempo contato avanza di un ms in più ogni 5000 tick.
- **Verifica:** test native su 10⁶ tick con ±1000, ±200, 0 ppm; test del calcolo da misura. La precisione reale si misura con `tools/calibrate.py` (C-29).

### C-8 — `lib/r080_core/Precision.{h,cpp}` (NUOVO): tabelle e F-Stop
- **Soddisfa:** REQ-4.1, REQ-4.2, REQ-4.3, REQ-4.5, REQ-3.10
- **Stato attuale:** `FSTOP_PRECISIONS[]` e `SECONDS_PRECISIONS[]` in `include/types.h` (stringhe PROGMEM `Pr:1/3  `…); `FStopState::getCalculatedTime()` in `include/functions/fstop.h` usa `pow(2.0, stopOffset/precisionValue)` con clamp 1..9999.
- **Modifica:** tabelle `kFStopDenominators[11] = {1,2,3,4,6,8,12,16,24,32,64}` e `kSecondsSteps[11] = {1,2,3,4,5,6,7,8,10,15,20}`; funzioni pure `uint16_t fstopTime(uint16_t baseDs, int8_t offset, uint8_t denom)` (arrotondamento al decimo, clamp `DS_MIN..DS_MAX`) e `uint16_t secondsStep(uint16_t ds, int8_t dir, uint8_t secs)`. Si usa `exp2f`/`powf` in float: la formula resta quella attuale.
- **Comportamento atteso:** stessi valori di oggi per base, offset e precisione.
- **Verifica:** test native con tabella di casi (base 10,0 s, 1/3, offset +1 → 12,6 s; offset +2 → 15,9 s; clamp ai limiti).

### C-9 — `lib/r080_core/TestStripPlan.{h,cpp}` (NUOVO): step del test strip
- **Soddisfa:** REQ-3.9, REQ-3.10, REQ-3.12, REQ-3.13, REQ-3.14, REQ-7.6, REQ-7.8
- **Stato attuale:** `onTestStripStep()` (`include/functions/timer_manager.h`) calcola il target successivo da `elapsedMs/100` arrotondato (`timerCurrentDecis * 2^(1/p)`), accumulando l'errore di arrotondamento; dopo uno stop manuale `g_elapsedMs` non torna a zero.
- **Modifica:** `struct TestStripPlan { uint16_t baseDs; PrecisionKind kind; uint8_t value; uint32_t stepMs(uint8_t k) const; }` con `stepMs(0) = T`, secondi `T + k·P`, F-Stop `T·2^(k/p)` calcolato sempre dalla base; garantisce passi strettamente crescenti di almeno 0,1 s (con 1/64 e base piccola l'arrotondamento produrrebbe step uguali); limite 999,9 s. Ogni avvio dopo Annullamento o conclusione riparte da `k = 0`.
- **Comportamento atteso:** sequenze 10,0 → 14,1 → 20,0 → 28,3 s (F-Stop 1/2) senza deriva.
- **Verifica:** test native su sequenze secondi e F-Stop, base minima, limite 999,9 s, ripartenza.

### C-10 — `lib/r080_core/Settings.{h,cpp}`, `Crc8.h` (NUOVO): Canali e formato EEPROM
- **Soddisfa:** REQ-2.5, REQ-6.1, REQ-6.4, REQ-6.5, REQ-6.6, REQ-6.7, REQ-6.8, REQ-7.2, REQ-7.12, REQ-9.3, REQ-9.7, REQ-9.8, REQ-9.11
- **Stato attuale:** `include/functions/storage.h` usa magic `0xA5` all'indirizzo 0 e 13 byte (luminosità, mute, canale, 3 tempi, 3 abilitazioni relè), senza CRC né versione; modalità e precisione non sono salvate.
- **Modifica:** nuovo layout con due blocchi indipendenti, ciascuno con il proprio CRC8 (polinomio 0x07):
  ```cpp
  struct ChannelRecord { uint8_t mode, precisionKind, precisionIndex, advance;
                         uint16_t timeDs, baseDs; int8_t offset; uint8_t relayEnabled; };   // 10 B
  struct GlobalRecord  { uint8_t activeChannel, mute, lcdLevel, segLevel,
                         lcdExposureLevel, focusLimitIndex; int16_t clockPpm; };              // 8 B
  // Blocco impostazioni @0x000: 'R','8', version=1, length | GlobalRecord | ChannelRecord[3] | crc8  = 43 B
  // Blocco contatori    @0x040: 'R','C', version=1, length | powerOns, watchdogResets (u16)
  //                             | exposures (u32) | lampSeconds (u32) | crc8                        = 17 B
  ```
  `SettingsCodec::load()` restituisce i valori di fabbrica se magic, versione, lunghezza o CRC non tornano (anche con il vecchio formato `0xA5`); `save()` passa solo i byte cambiati a `Storage::update()`. Le modifiche segnano le impostazioni come "da salvare" e `Settings::service(nowMs)` salva dopo 2 s senza nuove modifiche o subito all'avvio di un'Esposizione. Lo stesso ritardo vale per tutte le modifiche, non solo per le cifre. Valori di fabbrica: Canale A, tutti i Canali in Countdown, Secondi 1 s, continuo, 10,0 s, relè abilitato; mute spento; Focus 5 minuti; correzione 0 ppm; luminosità 7 segmenti livello 3 (valore 10), LCD livello 3, LCD in esposizione livello 1 (REQ-7.12).
- **Comportamento atteso:** al riavvio si ritrova lo stato salvato; una EEPROM corrotta o vecchia porta ai valori di fabbrica.
- **Verifica:** test native con `FakeStorage`: andata e ritorno, CRC errato, versione errata, formato `0xA5`, salvataggio ritardato di 2 s, nessuna scrittura se nulla cambia.

### C-11 — `lib/r080_core/KeyEntry.{h,cpp}` (NUOVO): inserimento a calcolatrice
- **Soddisfa:** REQ-5.1, REQ-5.2, REQ-5.3
- **Stato attuale:** in `evaluateKey()` (`include/functions/keypad_mgmt.h`) `nCifra` conta fino a 3 e poi continua a moltiplicare per 10 (overflow di `int` a 16 bit alla sesta cifra); non si azzera dopo un'Esposizione.
- **Modifica:** `class KeyEntry { uint16_t push(uint8_t digit); void breakEntry(); }`: `push` restituisce `(prev % 1000) * 10 + digit` se l'inserimento è in corso, altrimenti `digit`; `breakEntry()` è chiamato dal Controller (C-12) a ogni altra azione.
- **Comportamento atteso:** 1, 2, 5 → 0,1 / 1,2 / 12,5 s; 1, 2, 3, 4, 5 → 234,5 s; dopo un'Esposizione la prima cifra inizia un tempo nuovo.
- **Verifica:** test native.

### C-12 — `lib/r080_core/Controller.{h,cpp}` (NUOVO): macchina a stati dell'interfaccia
- **Soddisfa:** REQ-2.4, REQ-3.1, REQ-3.2, REQ-3.5, REQ-3.6, REQ-3.7, REQ-3.8, REQ-3.12, REQ-3.13, REQ-3.15, REQ-4.2, REQ-4.3, REQ-4.5, REQ-4.6, REQ-4.7, REQ-5.4, REQ-5.5, REQ-5.6, REQ-5.7, REQ-5.8, REQ-5.9, REQ-5.10, REQ-6.2, REQ-6.3, REQ-8.1, REQ-3.16
- **Stato attuale:** la logica è sparsa tra `manageMainButtonAction()`, `onStop()`, `evaluateKey()`, `evaluateEncoderMod()`, `evaluateEncoderPrec()`, `onModeButtonEvent()`, `onPrecisionButtonEvent()`, la classe `Operation` con puntatori a funzione (`showPresentation`, `switchModality`, `switchPrecisionType`, `startedAction`) e i flag `isMainBtnPressed`/`isActionStarted`.
- **Modifica:** un'unica macchina a stati che riceve eventi e produce comandi:
  ```cpp
  enum class Input : uint8_t { Digit, Star, Hash, StarHold, HashHold, ZeroStar, ZeroHash,
                               ChannelA, ChannelB, ChannelC, KeyD, ModeTurn, PrecTurn,
                               ModeClick, PrecClick, MainPress };
  enum class State : uint8_t { Idle, Running, StripPaused, Service };
  class Controller {
   public:
    void onInput(Input in, int8_t arg);          // arg: cifra o verso di rotazione
    void onTiming(uint8_t events);                // eventi da TimingCore
    // dipendenze iniettate: TimingPort (comandi a TimingCore in modo atomico), Sound, Screen, Settings, KeyEntry
  };
  ```
  Regole principali: in `Running` si accetta solo `MainPress` (REQ-5.9); `MainPress` in `Idle` con tempo 0 in Countdown/Test strip → beep di errore; `MainPress` in `Running` → Annullamento; in `StripPaused` → incremento successivo; `KeyD` azzera tempo, base e offset; `StarHold`/`HashHold` → mute o Abilitazione relè con beep di conferma; `ZeroStar`/`ZeroHash` e `Star`/`Hash` in Focus → luminosità 7 segmenti e LCD; `ModeTurn` cambia modalità senza toccare il tempo; `A/B/C` caricano l'intero `ChannelRecord`. In `StripPaused`, `KeyD`, `A/B/C` e `ModeTurn` concludono il test strip: `TimingCore::cancel()`, 7 segmenti al Tempo base, piano riportato a `k = 0` (REQ-3.16).
- **Comportamento atteso:** tutte le regole di interfaccia di EARS REQ-3, REQ-4, REQ-5 e REQ-6 sono osservabili con driver finti.
- **Verifica:** test native scenario per scenario con `FakeTimingPort`, `FakeBuzzer`, `FakeLcd`, `FakeSevenSeg`, `FakeStorage`.

### C-13 — `lib/r080_core/Screen.{h,cpp}` (NUOVO): modello e rendering dei display
- **Soddisfa:** REQ-2.6, REQ-2.7, REQ-4.4, REQ-7.1, REQ-7.3, REQ-7.4, REQ-7.5, REQ-7.6, REQ-7.7, REQ-7.8, REQ-7.9, REQ-7.10, REQ-3.7
- **Stato attuale:** `include/functions/presentation.h` scrive direttamente sull'LCD a ogni chiamata (`showPrecision()`, `showModalityType()`…), `showFstopOffset()` scrive alla colonna 5 sovrascrivendo il denominatore; `showTime()` scrive sul 7 segmenti a ogni giro da fermo.
- **Modifica:** `Screen` costruisce l'immagine voluta (2×16 caratteri, valore del 7 segmenti, luminosità) dallo stato del Controller, la confronta con l'ultima inviata e manda solo le differenze, al massimo una riga per giro di loop. Layout riga 2: colonne 0–7 precisione (`Pr:1/3  `, `Pr: 5s  `) oppure `St:n` durante il test strip, colonne 6–8 offset F-Stop (`+02`), colonne 9–12 Canale (`Ch:A`), colonna 14 nota del mute, colonna 15 icona del relè. In Focus le colonne 0–8 restano vuote. Durante l'Esposizione chiede all'LCD la luminosità di esposizione. Se `Lcd::online()` è falso continua ad aggiornare solo il 7 segmenti. Il 7 segmenti si aggiorna su `EV_FRAME` (10 Hz) durante l'Esposizione e solo al cambio di valore da fermo. La schermata di avvio (`R080 Timer` + `FW_VERSION`, 1,5 s) è uno stato temporizzato, non un `delay()`.
- **Comportamento atteso:** l'offset resta visibile accanto alla precisione; da fermo il bus I²C e la seriale restano inattivi se nulla cambia.
- **Verifica:** test native su `FakeLcd` (contenuto atteso cella per cella, nessuna scrittura senza cambiamenti, offset alle colonne 6–8).

### C-14 — `lib/r080_core/SoundScheduler.{h,cpp}` (NUOVO): suoni
- **Soddisfa:** REQ-1.5, REQ-1.6, REQ-3.4, REQ-8.1, REQ-8.2, REQ-8.3, REQ-8.4, REQ-8.5, REQ-8.6, REQ-8.7
- **Stato attuale:** `include/functions/sound.h`: `emitSound()` diretto e `SoundPlayer` con la sola `STOP_SEQUENCE` (300 Hz 75/150/75/150/150 ms), basato su `readTickMs()`; toni `TONE_UP = 600`, `TONE_DOWN = 300`.
- **Modifica:** sequenze costanti (`Start`, `Metronome`, `Step`, `PreWarn`, `End`, `Error`, `Confirm`, `FocusWarn`) come array di `{hz, ms}`; priorità `End` > `Error` > `Step` > `PreWarn` > `Metronome` > `Confirm`; con il mute suona solo `End`. `tick(nowMs)` avanza la sequenza attiva tramite `Buzzer`. Valori in un'unica tabella `kSounds` (REQ-8.7):
  ```cpp
  // {hz, ms}; hz = 0 è una pausa
  Start     = {{1000, 60}};
  Metronome = {{300, 80}};
  Step      = {{600, 150}};
  PreWarn   = {{800, 30}};                                        // emesso a -300, -200, -100 ms dallo Step
  End       = {{600, 150}, {0, 80}, {450, 150}, {0, 80}, {300, 150}};
  Error     = {{150, 300}};
  Confirm   = {{1200, 40}};
  FocusWarn = {{450, 200}, {0, 100}, {450, 200}};
  ```
- **Comportamento atteso:** ogni evento temporale produce un suono riconoscibile; con il mute resta solo il Suono di fine.
- **Verifica:** test native su `FakeBuzzer` (sequenze, priorità, mute); prova a orecchio nella Checklist di collaudo; registrazione audio per REQ-1.6.

### C-15 — `lib/r080_core/Diagnostics.{h,cpp}`, `ServiceMenu.{h,cpp}` (NUOVO): menu di servizio
- **Soddisfa:** REQ-9.1, REQ-9.2, REQ-9.3, REQ-9.4, REQ-9.5, REQ-9.6, REQ-9.7, REQ-9.8, REQ-9.9, REQ-9.10, REQ-9.11, REQ-9.12, REQ-9.14, REQ-3.3
- **Stato attuale:** non esiste.
- **Modifica:** `Diagnostics` tiene i massimi di durata del loop e della ISR e i tick persi (alimentati dai driver C-16 e C-24) e aggiorna i contatori del blocco contatori (C-10) a fine Esposizione: `exposures` +1 per ogni Countdown e per ogni test strip intero, `lampSeconds` sommando il tempo a relè acceso in qualsiasi modalità, Focus compreso (REQ-9.11); `powerOns` e `watchdogResets` all'avvio. `ServiceMenu` è uno stato del Controller (`State::Service`) con le voci Info, Contatori, Salute, Test hardware (relè, buzzer, 7 segmenti, LCD, tastierino, encoder, pulsante principale), Calibrazione (log di calibrazione, inserimento diretto in ppm, inserimento della durata misurata di un Countdown di 600,0 s con centesimi di secondo, convertita da `ppmFromMeasurement()`), Luminosità LCD in esposizione, Tempo massimo Focus (3/5/10/15/20 min), Ripristino di fabbrica, Esci (riavvio in uso normale). Navigazione (REQ-9.14): encoder della modalità per scorrere, suo pulsante per entrare o confermare, `D` per tornare indietro, cifre per i valori. `EventLog` formatta righe come `t=12345 START mode=CD ch=A ms=10000` e `t=22346 END ms=10000 measured=10000`.
- **Comportamento atteso:** il Menu di servizio si apre solo tenendo `D` all'accensione; ogni test agisce solo sul componente scelto.
- **Verifica:** test native della navigazione e dei calcoli; Checklist di collaudo per i test hardware.

### C-16 — `src/drivers/Timer1Clock.{h,cpp}` (NUOVO): Timer1, ISR e relè
- **Soddisfa:** REQ-1.1, REQ-1.2, REQ-1.3, REQ-2.1, REQ-9.4
- **Stato attuale:** `setupTimer1()` e `ISR(TIMER1_COMPA_vect)` in `include/functions/timer_isr.h`; macro `RELAY_ON()`/`RELAY_OFF()` su `PORTL` bit `PL2`; `relayStart()` in `include/functions/base.h`.
- **Modifica:** stessa configurazione dei registri (`TCCR1A = 0`, `TCCR1B = WGM12 | CS11 | CS10`, `OCR1A = 249`, `TIMSK1 |= OCIE1A`). La ISR chiama `core.tick()` (C-6), applica `RELAY_OFF()` se `relayOff`, accumula gli eventi in un `volatile uint8_t pendingEvents` e, in uscita, legge `TCNT1` per la durata massima della ISR. Comandi dal main (`start*`, `setNextStep`, `cancel`) e `takeEvents()` sono in `ATOMIC_BLOCK`. Lo start accende il relè con `RELAY_ON()` solo se l'Abilitazione relè del Canale è attiva, dopo aver armato il core. Tick persi: Timer4 in conteggio libero con prescaler 64 fa da riferimento hardware indipendente e il main confronta ogni secondo i due conteggi. In `setup()` `PL2` viene messo in uscita a livello basso come prima istruzione.
- **Comportamento atteso:** relè spento dalla ISR al ms esatto in Countdown, passo-passo e spegnimento automatico del Focus.
- **Verifica:** `avr-objdump -d` della ISR senza `call`; oscilloscopio o analizzatore logico su D47 (fine Countdown entro 1 ms); misura della durata della ISR dal Menu di servizio.

### C-17 — `src/drivers/LcdWaveshare.{h,cpp}` (NUOVO): driver LCD non bloccante
- **Soddisfa:** REQ-2.6, REQ-2.7, REQ-7.1, REQ-7.2, REQ-7.3, REQ-7.11, REQ-10.1
- **Stato attuale:** `Waveshare_LCD1602_RGB::send()` esegue `delay(5)` per byte; `setCursor()` + `write_char()` = 4 transazioni I²C per carattere; `init()` usa `delay(50)`/`delay(5)`; indirizzi `LCD_ADDRESS 0x3E`, `RGB_ADDRESS 0x60` (controller PCA9633, registri `REG_RED 0x04`, `REG_GREEN 0x03`, `REG_BLUE 0x02`, `REG_MODE1`, `REG_MODE2`, `REG_OUTPUT`).
- **Modifica:** `Wire.setClock(100000)` e `Wire.setWireTimeout(3000, true)`; `writeAt()` invia una sola transazione per una sequenza di caratteri contigui (byte di controllo `0x80` + indirizzo DDRAM, poi `0x40` + caratteri); nessun `delay` dopo l'inizializzazione, che usa una piccola macchina a stati con i tempi richiesti dal controller; colore sempre `setReg(REG_RED, kLcdLevels[level - 1])` con `kLcdLevels[8] = {4, 8, 16, 28, 48, 80, 140, 255}`, `REG_GREEN = REG_BLUE = 0`, `REG_MODE2` senza blink (`0x00`); `online()` diventa falso al primo errore di `endTransmission()` o timeout, con un nuovo tentativo ogni 2 s registrato nel log; caratteri personalizzati nota e spina caricati all'inizializzazione.
- **Comportamento atteso:** una riga intera costa una transazione di circa 2 ms; un LCD scollegato non blocca il loop.
- **Verifica:** misura della durata massima del loop dal Menu di servizio; prova con LCD scollegato durante un Countdown (Checklist).

### C-18 — `src/drivers/SevenSegSerial3.{h,cpp}` (NUOVO): 7 segmenti su Serial3
- **Soddisfa:** REQ-10.2, REQ-10.4, REQ-7.2, REQ-7.4, REQ-7.7, REQ-7.11
- **Stato attuale:** `SoftwareSerial sevenSegmentsDisplay(1, DISPLAY_7_SEG_PIN)` su A2 a 9600 baud (con pin RX 1 condiviso con `Serial`); comandi `0x76` (reset), `0x77` + `0b00000100` (punto decimale), `0x7A` + luminosità; `showTime()` invia `"%04i"`.
- **Modifica:** `Serial3.begin(9600)` sul pin 14 (TX3); stessi comandi del display; luminosità `0x7A` + `kSegLevels[level - 1]` con `kSegLevels[8] = {2, 5, 10, 20, 40, 80, 150, 255}` (il livello 3 riproduce il valore 10 di oggi); il buffer di trasmissione hardware da 64 byte rende le scritture non bloccanti; il driver scrive solo quando il valore cambia. Cablaggio: il filo DIN passa da A2 a D14.
- **Comportamento atteso:** nessuna disabilitazione degli interrupt durante l'aggiornamento del 7 segmenti.
- **Verifica:** jitter del Metronomo con registrazione audio (REQ-1.6); durata massima della ISR invariata durante l'Esposizione.

### C-19 — `src/drivers/BuzzerTone.{h,cpp}` (NUOVO): buzzer
- **Soddisfa:** REQ-8.1, REQ-8.4, REQ-8.6
- **Stato attuale:** `tone(BUZZER_PIN, …)` e `noTone()` su D49 (Timer2) in `include/functions/sound.h`.
- **Modifica:** implementazione di `Buzzer` con `tone()`/`noTone()` sullo stesso pin. `tone()` usa Timer2 e scrive su `PORTL` bit `PL0`, nello stesso registro del relè: le scritture del relè dal main stanno in `ATOMIC_BLOCK` per non perdere il bit del buzzer.
- **Comportamento atteso:** stessi suoni di oggi dove non cambiano.
- **Verifica:** Checklist di collaudo (ascolto di tutte le sequenze dal Menu di servizio).

### C-20 — `src/drivers/InputDevices.{h,cpp}` (NUOVO): tastierino, encoder e pulsanti
- **Soddisfa:** REQ-2.4, REQ-5.5, REQ-5.6, REQ-5.8, REQ-5.10, REQ-5.11, REQ-5.12
- **Stato attuale:** `Keypad` con `setDebounceTime(50)` e hold predefinito di 500 ms (`Keypad.cpp`, `setHoldTime(500)`); dopo un HOLD arriva comunque un `RELEASED` che esegue l'azione normale; `Encoder` letto come posizione assoluta (`read() >> 2` modulo 3 o 11); `EncoderButton` per i pulsanti degli encoder; `OneButton` con `attachClick`, `setClickMs(100)`.
- **Modifica:** `Keypad` con `setHoldTime(1000)`; per ogni tasto un flag "hold scattato" che sopprime l'evento al rilascio; combinazione: `*`/`#` rilasciati mentre `0` è premuto producono `ZeroStar`/`ZeroHash` e sopprimono il rilascio di `0`. Encoder: si legge la differenza rispetto alla lettura precedente, divisa per 4 (una tacca), e si genera `ModeTurn`/`PrecTurn` con il verso. Pulsanti: tre `OneButton` (pin 2, 17, 16) con `setDebounceMs(20)`; il principale usa `attachPress()`, che scatta all'ingresso nello stato premuto dopo l'antirimbalzo, gli altri `attachClick()`. Tutti gli input sono letti a ogni giro di loop, che resta breve grazie a C-13 e C-17.
- **Comportamento atteso:** start alla pressione, nessuna doppia azione dopo un HOLD, nessuna pressione persa.
- **Verifica:** test hardware del Menu di servizio; Checklist (pressioni brevi ripetute, HOLD, combinazioni).

### C-21 — `src/drivers/EepromStorage.{h,cpp}` (NUOVO): EEPROM
- **Soddisfa:** REQ-6.5, REQ-6.8
- **Stato attuale:** `eeprom_read_byte`/`eeprom_update_byte`/`eeprom_update_word` a indirizzi fissi in `include/functions/storage.h`.
- **Modifica:** implementazione di `Storage` con `eeprom_read_block()` e `eeprom_update_block()`.
- **Comportamento atteso:** scrive solo i byte cambiati.
- **Verifica:** contatore di scritture in un build di debug; Checklist (spegni e riaccendi).

### C-22 — `src/drivers/SystemGuard.{h,cpp}` (NUOVO): watchdog, brown-out e causa del reset
- **Soddisfa:** REQ-2.1, REQ-2.2, REQ-2.3, REQ-2.8, REQ-9.9
- **Stato attuale:** nessun watchdog. Il bootloader `stk500v2` legge `MCUSR`, lo azzera e, se il reset viene dal watchdog, salta subito all'applicazione: il firmware non può leggere la causa del reset da `MCUSR`.
- **Modifica:** watchdog in modalità interrupt + reset con timeout 250 ms (`WDTCSR = WDIE | WDE | WDP2`): al primo timeout `ISR(WDT_vect)` esegue `RELAY_OFF()` e scrive un marcatore in una variabile `__attribute__((section(".noinit")))`; al secondo timeout (500 ms totali) la scheda si resetta. All'avvio il marcatore distingue un reset da watchdog da una normale accensione e viene cancellato. Per il brown-out resta la soglia di fabbrica dei fuse (2,7 V): un reset da calo di tensione appare come una normale accensione. La soglia a 4,3 V via ISP è nel backlog. Nessuna Esposizione viene ripresa dopo un reset: lo stato iniziale è sempre `Idle` con relè spento.
- **Comportamento atteso:** un loop bloccato spegne il relè entro 250 ms e resetta entro 500 ms; dopo il reset compare `RESET: WDT` per 3 s.
- **Verifica:** build di test con un ciclo infinito comandato dal Menu di servizio; analizzatore logico su D47.

### C-23 — `src/drivers/UsbLog.{h,cpp}` (NUOVO): log USB
- **Soddisfa:** REQ-9.10, REQ-9.6
- **Stato attuale:** `Serial.begin(9600)` e il solo banner `=== R080 TIMER PRONTO ===`.
- **Modifica:** `Serial.begin(115200)`; implementazione di `EventLog` che scrive solo se c'è spazio in `Serial.availableForWrite()`, per non bloccare mai il loop (le righe che non entrano vengono contate come perse). Durante il log di calibrazione scrive una riga `CAL ms=<conteggio>` al secondo, allineata al tick di Timer1.
- **Comportamento atteso:** `pio device monitor -b 115200` mostra avvio, Esposizioni con durata misurata ed errori.
- **Verifica:** Checklist di collaudo.

### C-24 — `src/main.cpp`: composizione e loop
- **Soddisfa:** REQ-11.2, REQ-2.1, REQ-7.9, REQ-9.1, REQ-9.4
- **Stato attuale:** `setup()` con `delay(3000)`, `loadSettings()` seguito da `initializeOperations()` (che azzera il tempo); `loop()` con lettura atomica dei flag `g_*` e `manageMainButtonAction()`.
- **Modifica:** `setup()`: relè in uscita a livello basso, `SystemGuard`, lettura del tasto `D` per il Menu di servizio, caricamento impostazioni, inizializzazione driver, avvio di Timer1 e Timer4. `loop()`: `wdt_reset()`, `takeEvents()`, lettura input, `Controller`, `SoundScheduler::tick()`, `Screen::render()`, `Settings::service()`, misura della durata del loop per `Diagnostics`. Pin in `src/drivers/Pins.h` (NUOVO), con gli stessi valori di `include/constants.h` tranne il 7 segmenti.
- **Comportamento atteso:** nessun `delay()` dopo l'avvio; loop breve in ogni stato.
- **Verifica:** durata massima del loop dal Menu di servizio; Checklist completa.

### C-25 — `test/` (NUOVO): test native
- **Soddisfa:** REQ-11.3
- **Stato attuale:** `test/README` di PlatformIO, nessun test.
- **Modifica:** una cartella per modulo (`test_timing_core`, `test_clock_correction`, `test_precision`, `test_test_strip`, `test_settings`, `test_key_entry`, `test_controller`, `test_screen`, `test_sound`, `test_service_menu`) con Unity e driver finti in `test/fakes/` (`FakeLcd`, `FakeSevenSeg`, `FakeBuzzer`, `FakeStorage`, `FakeTimingPort`, `FakeLog`). I test si scrivono prima del codice (TDD), modulo per modulo.
- **Comportamento atteso:** `pio test -e native` copre tutta la logica di dominio.
- **Verifica:** esecuzione locale e in CI (C-3).

### C-26 — `CLAUDE.md` e documenti per agenti
- **Soddisfa:** REQ-11.1, REQ-12.4
- **Stato attuale:** `CLAUDE.md` §3 descrive "Single Translation Unit", ordine degli include con `timer_isr.h` prima di `base.h`, `channel` 1..3.
- **Modifica:** nuovi invarianti: tre strati, dominio senza Arduino, niente blocchi nel loop, ISR senza divisioni, `ATOMIC_BLOCK`, `eeprom_update_*`, niente allocazione dinamica, comandi `pio test -e native`; nomi dei branch nuovi (vedi C-1).
- **Comportamento atteso:** un agente che legge `CLAUDE.md` trova regole coerenti con il codice nuovo.
- **Verifica:** revisione.

### C-27 — Documentazione utente e tecnica
- **Soddisfa:** REQ-12.1, REQ-12.3, REQ-12.5, REQ-10.5
- **Stato attuale:** `README.md`, `README-IT.md` (citano ancora `ArduinoSTL`), `DOCS.md`, `DOCS-IT.md`, `TECNICO.md`, `DIAGRAMS.md`, `PINOUT.md` e i PDF `TECNICO.pdf`, `DIAGRAMS.pdf`, `PINOUT.pdf`.
- **Modifica:** manuale utente e README riscritti in italiano e inglese; `PINOUT.md` aggiornato (7 segmenti su D14, pull-down da 10 kΩ su D47); il manuale descrive la calibrazione con `tools/calibrate.py` e, come alternativa senza PC, con la registrazione audio dei beep di un Countdown di 600,0 s; `TECNICO.md` e `DIAGRAMS.md` sostituiti da documentazione tecnica nuova; PDF rimossi; `CHANGELOG.md` (NUOVO) da `v2.0.0`; `docs/refactoring-timing/` intatto.
- **Comportamento atteso:** documentazione coerente con il firmware rilasciato.
- **Verifica:** revisione prima del rilascio.

### C-28 — `docs/firmware-rewrite/checklist-collaudo.md` (NUOVO): rilascio
- **Soddisfa:** REQ-12.1, REQ-12.2, REQ-1.4, REQ-1.6
- **Stato attuale:** checklist per fase in `docs/refactoring-timing/verify/`, mai compilate.
- **Modifica:** una Checklist di collaudo unica con le prove su scheda di ogni requisito non coperto dai test native (misure di tempo, jitter del Metronomo, luci, suoni, guasti LCD, watchdog, persistenza), spazio per esiti e firma; il rilascio è il tag `vX.Y.Z` sul commit collaudato.
- **Comportamento atteso:** ogni versione rilasciata ha una checklist firmata.
- **Verifica:** presenza della checklist firmata per il tag `v2.0.0`.

### C-29 — `tools/calibrate.py` (NUOVO): calibrazione dal PC
- **Soddisfa:** REQ-9.13, REQ-1.4
- **Stato attuale:** non esiste.
- **Modifica:** script Python (solo libreria standard più `pyserial`) che apre la porta USB a 115200 baud, legge le righe `CAL ms=<conteggio>` per 30 minuti, registra per ciascuna l'istante dell'orologio del PC (`time.monotonic_ns()`), stima la pendenza con una regressione lineare (che assorbe il jitter USB) e stampa la Correzione del clock in ppm da inserire nel Menu di servizio.
- **Comportamento atteso:** con il quarzo del Mega stampa un valore tipicamente entro ±50 ppm, con incertezza di pochi ppm.
- **Verifica:** test dello script con un log sintetico a deriva nota; prova reale sulla scheda (Checklist).

### C-30 — Cablaggio: pull-down sul relè
- **Soddisfa:** REQ-10.5, REQ-2.1
- **Stato attuale:** D47 collegato direttamente all'ingresso del modulo relè; il modulo potrebbe avere già una resistenza di pull-down propria.
- **Modifica:** resistenza da 10 kΩ tra D47 e GND. Il modulo è attivo alto (il firmware accende con `RELAY_ON()` = livello alto), quindi la resistenza è innocua anche in parallelo a una già presente.
- **Comportamento atteso:** relè spento dal momento dell'accensione fino al primo comando del firmware.
- **Verifica:** analizzatore logico o LED su D47 durante 10 accensioni consecutive (Checklist).

## Ordine di implementazione

1. **C-1** branch e tag.
2. **C-2** `platformio.ini` con ambiente native; **C-3** CI. Il firmware vecchio continua a compilare.
3. Dominio con test scritti prima del codice (**C-25** cresce insieme a ogni modulo): **C-5** → **C-8** → **C-7** → **C-6** → **C-9** → **C-10** → **C-11** → **C-14** → **C-13** → **C-12** → **C-15**. La libreria non è ancora usata dal firmware vecchio, che resta compilabile.
4. Driver: **C-16**, **C-17**, **C-18**, **C-19**, **C-20**, **C-21**, **C-22**, **C-23**, ciascuno compilato con un piccolo `main` di prova nell'ambiente Mega; in parallelo **C-30** (cablaggio) e **C-29** (script di calibrazione).
5. **C-24** nuovo `src/main.cpp` insieme a **C-4** rimozione del codice vecchio, nello stesso commit.
6. **C-26** `CLAUDE.md`; **C-27** documentazione; **C-28** checklist e rilascio `v2.0.0`.

## Rischi tecnici

| Rischio | Impatto | Mitigazione |
|---|---|---|
| Il bootloader cancella `MCUSR` e la soglia di brown-out di fabbrica è 2,7 V, sotto i 4,5 V richiesti a 16 MHz | Comportamento imprevedibile tra 4,5 e 2,7 V; calo di tensione non riconosciuto | Marcatore `.noinit` per il watchdog; soglia a 4,3 V via ISP nel backlog |
| Pin 47 in alta impedenza durante il bootloader | Lampada accesa per un istante all'accensione | Pull-down da 10 kΩ (C-30) |
| Jitter USB e latenza del PC durante la calibrazione | Errore sulla correzione | Regressione lineare su 30 minuti di campioni (C-29) |
| Vtable delle interfacce in RAM su AVR | Qualche decina di byte | Poche interfacce; misura con `pio run` (oggi 17,9% di RAM) |
| `float`/`powf` sul Mega per F-Stop | Flash e tempo di calcolo | Calcolo solo nel loop, mai nella ISR; risultati verificati nei test native |
| Usura EEPROM dei contatori (byte meno significativo scritto a ogni Esposizione, ~100 000 cicli) | Contatori inaffidabili dopo ~100 000 Esposizioni | Accettabile per la v1; anello di slot se servisse |
| `PORTL` condiviso da relè (`PL2`) e buzzer (`PL0`, scritto da `tone()` in ISR) | Perdita di un fronte del buzzer | Scritture del relè dal main in `ATOMIC_BLOCK` (C-19) |
| Sorpresa nella compatibilità di avr-gcc 7.3 con `-std=gnu++17` | Build fallita | Verifica nel primo commit di C-2; ripiego su `gnu++14` |
| Wire con timeout attivo resetta il bus I²C a metà transazione | Caratteri sporchi sull'LCD | `Screen` ridisegna l'intera immagine dopo ogni errore |

## Matrice di tracciabilità

| REQ | Modifiche | Verifica |
|---|---|---|
| REQ-1.1 | C-6, C-16 | Test native; ispezione registri |
| REQ-1.2 | C-6, C-16 | Test native; analizzatore logico su D47 |
| REQ-1.3 | C-6, C-16 | Test native; analizzatore logico su D47 |
| REQ-1.4 | C-7, C-28, C-29 | Misura esterna su 60,0 s dopo la calibrazione |
| REQ-1.5 | C-6, C-14 | Test native; registrazione audio |
| REQ-1.6 | C-6, C-14, C-28 | Test native; registrazione audio, σ < 1 ms |
| REQ-1.7 | C-7 | Test native |
| REQ-2.1 | C-16, C-22, C-24, C-30 | Analizzatore logico all'accensione |
| REQ-2.2 | C-22 | Build di test con loop bloccato |
| REQ-2.3 | C-22 | Alimentatore regolabile sotto 2,7 V |
| REQ-2.4 | C-12, C-20 | Checklist (rimbalzi) |
| REQ-2.5 | C-10 | Test native |
| REQ-2.6 | C-13, C-17 | Checklist: LCD scollegato durante il Countdown |
| REQ-2.7 | C-13, C-17 | Durata massima del loop con LCD scollegato |
| REQ-2.8 | C-22 | Checklist: togliere corrente durante un'Esposizione |
| REQ-3.1 | C-12 | Test native |
| REQ-3.2 | C-12 | Test native; Checklist |
| REQ-3.3 | C-6, C-15 | Test native |
| REQ-3.4 | C-6, C-14 | Test native |
| REQ-3.5 | C-12 | Test native |
| REQ-3.6 | C-12 | Test native |
| REQ-3.7 | C-12, C-13 | Test native |
| REQ-3.8 | C-12 | Test native |
| REQ-3.9 | C-9 | Test native |
| REQ-3.10 | C-8, C-9 | Test native |
| REQ-3.11 | C-6 | Test native |
| REQ-3.12 | C-6, C-9, C-12 | Test native |
| REQ-3.13 | C-9, C-12 | Test native |
| REQ-3.14 | C-6, C-9 | Test native |
| REQ-3.15 | C-12 | Test native |
| REQ-3.16 | C-12 | Test native |
| REQ-4.1 | C-8 | Test native |
| REQ-4.2 | C-8, C-12 | Test native |
| REQ-4.3 | C-8, C-12 | Test native |
| REQ-4.4 | C-13 | Test native su `FakeLcd` |
| REQ-4.5 | C-8, C-12 | Test native |
| REQ-4.6 | C-12 | Test native |
| REQ-4.7 | C-12 | Test native |
| REQ-5.1 | C-11 | Test native |
| REQ-5.2 | C-11 | Test native |
| REQ-5.3 | C-11 | Test native |
| REQ-5.4 | C-12 | Test native |
| REQ-5.5 | C-12, C-20 | Test native; Checklist |
| REQ-5.6 | C-12, C-20 | Test native; Checklist |
| REQ-5.7 | C-12 | Test native |
| REQ-5.8 | C-12, C-20 | Test native; Checklist |
| REQ-5.9 | C-12 | Test native |
| REQ-5.10 | C-12, C-20 | Test native; Checklist |
| REQ-5.11 | C-20 | Checklist |
| REQ-5.12 | C-20 | Checklist; durata massima del loop |
| REQ-6.1 | C-10 | Test native |
| REQ-6.2 | C-12 | Test native |
| REQ-6.3 | C-12 | Test native |
| REQ-6.4 | C-10 | Test native; Checklist (spegni e riaccendi) |
| REQ-6.5 | C-10, C-21 | Test native |
| REQ-6.6 | C-10 | Test native |
| REQ-6.7 | C-10 | Test native |
| REQ-6.8 | C-10, C-21 | Test native |
| REQ-7.1 | C-13, C-17 | Checklist |
| REQ-7.2 | C-10, C-17, C-18 | Test native; Checklist |
| REQ-7.3 | C-13, C-17 | Test native; Checklist |
| REQ-7.4 | C-13, C-18 | Checklist |
| REQ-7.5 | C-13 | Test native su `FakeLcd` |
| REQ-7.6 | C-9, C-13 | Test native |
| REQ-7.7 | C-13, C-18 | Test native |
| REQ-7.8 | C-9, C-13 | Test native |
| REQ-7.9 | C-13, C-24 | Checklist |
| REQ-7.10 | C-13 | Test native |
| REQ-7.11 | C-13, C-17, C-18 | Checklist (scala percepita al buio) |
| REQ-7.12 | C-10 | Test native |
| REQ-8.1 | C-12, C-14, C-19 | Test native; Checklist |
| REQ-8.2 | C-14 | Test native |
| REQ-8.3 | C-6, C-14 | Test native |
| REQ-8.4 | C-14, C-19 | Test native; Checklist |
| REQ-8.5 | C-14 | Test native |
| REQ-8.6 | C-14, C-19 | Checklist (ascolto) |
| REQ-8.7 | C-14 | Test native su `FakeBuzzer` |
| REQ-9.1 | C-15, C-24 | Checklist |
| REQ-9.2 | C-15 | Checklist |
| REQ-9.3 | C-10, C-15 | Test native |
| REQ-9.4 | C-15, C-16, C-24 | Checklist |
| REQ-9.5 | C-15 | Checklist |
| REQ-9.6 | C-15, C-23 | Checklist con `pio device monitor` |
| REQ-9.7 | C-10, C-15 | Test native |
| REQ-9.8 | C-10, C-15 | Test native |
| REQ-9.9 | C-15, C-22 | Build di test con loop bloccato |
| REQ-9.10 | C-15, C-23 | Checklist con `pio device monitor` |
| REQ-9.11 | C-10, C-15 | Test native |
| REQ-9.12 | C-7, C-15 | Test native |
| REQ-9.13 | C-29 | Test con log sintetico; Checklist |
| REQ-9.14 | C-15 | Test native |
| REQ-10.1 | C-17 | Checklist |
| REQ-10.2 | C-18 | Checklist |
| REQ-10.3 | C-2, C-4 | `pio run`; ispezione di `platformio.ini` |
| REQ-10.4 | C-18, C-30 | Confronto con `PINOUT.md` |
| REQ-10.5 | C-27, C-30 | Ispezione del cablaggio |
| REQ-11.1 | C-4, C-5, C-26 | `grep` degli include Arduino in `lib/r080_core` |
| REQ-11.2 | C-5, C-24 | Revisione del codice |
| REQ-11.3 | C-2, C-6, C-25 | `pio test -e native` |
| REQ-11.4 | C-3 | Workflow su GitHub |
| REQ-11.5 | C-2, C-5 | Test native |
| REQ-12.1 | C-27, C-28 | Presenza di `CHANGELOG.md` e tag |
| REQ-12.2 | C-28 | Checklist firmata per il tag |
| REQ-12.3 | C-27 | Revisione |
| REQ-12.4 | C-1, C-26 | `git ls-remote` |
| REQ-12.5 | C-27 | Revisione del manuale |

## Questioni aperte

Nessuna questione aperta: tutte sono state risolte nel giro 7 della sessione di grilling.

- **Q-1 — Causa del reset e soglia di brown-out:** risolta (soglia di fabbrica, solo watchdog; ISP nel backlog). Vedi C-22.
- **Q-2 — Parametri dei suoni:** risolta. Vedi C-14.
- **Q-3 — Scale di luminosità:** risolta. Vedi C-10, C-17, C-18.
- **Q-4 — Fine di un test strip passo-passo in pausa:** risolta. Vedi C-12.
- **Q-5 — Cosa conta come Esposizione nei contatori:** risolta. Vedi C-15.
- **Q-6 — Relè durante il bootloader:** risolta. Vedi C-30.
- **Q-7 — Precisione della calibrazione:** risolta. Vedi C-15, C-23, C-29.
- **Q-8 — Navigazione del Menu di servizio:** risolta in REQ-9.14. Vedi C-15.
