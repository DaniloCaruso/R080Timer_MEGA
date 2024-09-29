# R080 TIMER MEGA - Manuale d'Uso e Documentazione Tecnica

## Indice

1. [Panoramica](#panoramica)
2. [Hardware](#hardware)
3. [Controlli](#controlli)
4. [Display](#display)
5. [Modalita Operative](#modalita-operative)
6. [Navigazione F-Stop](#navigazione-f-stop)
7. [Canali](#canali)
8. [Persistenza EEPROM](#persistenza-eeprom)
9. [Suono](#suono)
10. [Architettura Software](#architettura-software)
11. [Riferimento Rapido Comandi](#riferimento-rapido-comandi)

---

## Panoramica

R080 Timer MEGA e' un timer digitale di precisione per ingranditore fotografico in camera oscura. Basato su Arduino Mega 2560, offre tre modalita operative (Focus, Countdown, Test Strip), navigazione F-Stop, 3 canali con memoria indipendente e persistenza delle impostazioni.

**Unita di tempo interna:** decisecondi (1/10 di secondo). Il valore `100` sul display equivale a `10.0` secondi.

---

## Hardware

| Componente               | Connessione                        |
|--------------------------|-------------------------------------|
| Arduino Mega 2560        | Scheda principale                   |
| LCD RGB 16x2 (Waveshare) | I2C (addr 0x3E/0x60)              |
| Display 7 segmenti       | SoftwareSerial su pin A2 (9600 baud)|
| Keypad 4x4               | Colonne: 41, 39, 37, 35 / Righe: 33, 31, 29, 27 |
| Encoder modalita          | DT=3, CLK=4, pulsante SW=17       |
| Encoder precisione        | DT=18, CLK=19, pulsante SW=16     |
| Pulsante principale       | Pin 2 (INPUT_PULLUP)              |
| Buzzer                    | Pin 49                             |
| Relay ingranditore        | Pin 47                             |

---

## Controlli

### Pulsante Principale (pin 2)

- **Pressione singola:** Avvia/Ferma l'operazione corrente (toggle).
- Quando avviato: il relay si attiva (se abilitato per il canale) e il timer parte.
- Quando fermato: il relay si disattiva e il timer si ferma.

### Encoder Modalita (sinistro)

- **Rotazione:** Cicla tra le 3 modalita: FOCUS -> COUNTDOWN -> TEST STRIP -> FOCUS...
- **Pressione pulsante:** In TEST STRIP, alterna la modalita di avanzamento:
  - `CO` = Continuo (esposizioni consecutive senza pausa)
  - `PP` = Passo-Passo (si ferma ad ogni step, ripremi per continuare)

### Encoder Precisione (destro)

- **Rotazione:** Cambia il valore di precisione tra 11 livelli.
  - In F-Stop: 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64
  - In Secondi: 1s, 2s, 3s, 4s, 5s, 6s, 7s, 8s, 10s, 15s, 20s
- **Pressione pulsante:** Alterna tra tipo di precisione F-Stop (`F`) e Secondi (`S`).

### Keypad 4x4

Layout fisico:

```
 1  2  3  A
 4  5  6  B
 7  8  9  C
 *  0  #  D
```

---

## Display

### Display 7 Segmenti

Mostra il tempo corrente in decisecondi con 4 cifre e punto decimale sulla prima cifra.

Esempi:
- `10.00` = 100.0 secondi (1000 decisecondi)
- ` 5.00` = 50.0 secondi
- ` 0.30` = 3.0 secondi

### LCD 16x2

Il layout dipende dalla modalita attiva:

**FOCUS:**
```
Riga 0: FOCUS
Riga 1:          Ch:A ♪⚡
```
Nessuna precisione mostrata. `♪` = suono attivo, `⚡` = relay abilitato.

**COUNTDOWN:**
```
Riga 0: COUNTDOWN  [S]
Riga 1: Pr: 5s    Ch:A ♪⚡
```
`[S]` = tipo Secondi, `[F]` = tipo F-Stop.

**COUNTDOWN in F-Stop:**
```
Riga 0: COUNTDOWN  [F]
Riga 1: Pr:1/3+02 Ch:A ♪⚡
```
`+02` = 2 stop sopra il tempo base. `-01` = 1 stop sotto. ` 00` = al tempo base.

**TEST STRIP:**
```
Riga 0: TEST STRIP [S] CO
Riga 1: Pr: 5s    Ch:B ♪⚡
```
`CO` = Continuo, `PP` = Passo-Passo.

---

## Modalita Operative

### 1. FOCUS

**Scopo:** Proiezione continua per messa a fuoco e composizione.

**Comportamento:**
- Premendo il pulsante: il relay si attiva e il timer conta in avanti da 0 (incremento ogni 0.1s).
- Il tempo mostrato sul display cresce continuamente.
- Premendo di nuovo: relay si disattiva e timer si ferma.

**Controlli speciali in FOCUS:**
- Tasto `*` = diminuisce luminosita display 7 segmenti (da 10 a 1)
- Tasto `#` = aumenta luminosita display 7 segmenti (da 1 a 10)
- I tasti numerici sono disabilitati.

### 2. COUNTDOWN

**Scopo:** Esposizione cronometrata con conto alla rovescia.

**Preparazione:**
1. Digitare il tempo desiderato con il keypad numerico (fino a 4 cifre, in decisecondi).
   - Esempio: `1` `0` `0` = 10.0 secondi
   - Esempio: `5` = 0.5 secondi
   - Esempio: `5` `0` = 5.0 secondi
2. Oppure usare `*`/`#` per navigare il tempo (vedi sezione F-Stop o Secondi).

**Comportamento:**
- Premendo il pulsante: relay si attiva, timer conta alla rovescia.
- **Metronomo:** ogni secondo (ogni 10 decisecondi) viene emesso un beep.
- A tempo zero: relay si disattiva, suono di stop (3 beep), il timer si ferma automaticamente.
- **Repeat last exposure:** al termine, il tempo viene ripristinato al valore di partenza. Puoi ripartire immediatamente premendo il pulsante.
- Premendo il pulsante durante il conto: interrompe l'esposizione.

**Navigazione tempo:**

Con tipo **Secondi** (`S`):
- `#` = aggiunge `precisione * 10` decisecondi al tempo (es. precisione 5s -> +50 decisecondi)
- `*` = sottrae `precisione * 10` decisecondi (minimo 1)

Con tipo **F-Stop** (`F`):
- `#` = avanza di uno stop (vedi sezione F-Stop)
- `*` = torna indietro di uno stop
- Il display mostra l'offset dal tempo base (es. `+02`, `-01`, ` 00`)

### 3. TEST STRIP

**Scopo:** Esposizioni incrementali per provini di esposizione.

**Preparazione:**
1. Impostare il tempo base con il keypad (primo step di esposizione).
2. Scegliere la precisione (distanza tra gli step).
3. Scegliere la modalita: Continuo (`CO`) o Passo-Passo (`PP`).

**Comportamento - Continuo (`CO`):**
- Premendo il pulsante: relay si attiva, timer conta in avanti.
- Ad ogni step viene emesso un beep. 3 decisecondi prima dello step: beep di avviso.
- Il timer prosegue automaticamente allo step successivo.
- Premendo il pulsante: interrompe tutto.

**Comportamento - Passo-Passo (`PP`):**
- Premendo il pulsante: relay si attiva, timer conta fino al primo step.
- Allo step: relay si disattiva, timer si ferma, display mostra il prossimo target.
- L'utente puo' spostare la striscia di carta sotto l'ingranditore.
- Premendo di nuovo: riparte per il prossimo step (il timer continua da dove si era fermato).

**Calcolo step:**

Con tipo **Secondi:** step lineari. Es. con precisione 5s e base 50 decisecondi (5s):
- Step 1: 0 -> 50 (5.0s)
- Step 2: 50 -> 100 (10.0s)
- Step 3: 100 -> 150 (15.0s)

Con tipo **F-Stop:** step logaritmici. Es. con precisione 1/3 e base 50 decisecondi:
- Step 1: 0 -> 50
- Step 2: 50 -> 63 (50 * 2^(1/3) = 63)
- Step 3: 63 -> 79 (63 * 2^(1/3) = 79)

---

## Navigazione F-Stop

### Concetto

L'F-Stop e' un sistema logaritmico usato in fotografia. Raddoppiare il tempo equivale a +1 stop. Il timer permette frazioni di stop per regolazioni fini.

### Come funziona

1. **Imposta un tempo base** digitandolo con il keypad (es. `100` = 10 secondi).
2. **Premi il pulsante dell'encoder precisione** per passare a modalita F-Stop (`F`).
3. **Ruota l'encoder precisione** per scegliere la frazione: 1/1 (stop interi), 1/2 (mezzi stop), 1/3 (terzi di stop), ecc.
4. **Premi `#`** per salire di uno stop. **Premi `*`** per scendere.

### Esempio pratico

Tempo base: 100 decisecondi (10.0s), precisione 1/3 stop:

| Tasto | Offset | Calcolo | Tempo risultante |
|-------|--------|---------|-----------------|
| (base)| ` 00`  | 100     | 10.0s           |
| `#`   | `+01`  | 100 * 2^(1/3) = 126  | 12.6s  |
| `#`   | `+02`  | 100 * 2^(2/3) = 159  | 15.9s  |
| `#`   | `+03`  | 100 * 2^(3/3) = 200  | 20.0s  |
| `*`   | `+02`  | 100 * 2^(2/3) = 159  | 15.9s  |

La formula: `tempo = tempoBase * 2^(offset / precisione)`

### Note importanti

- Il tempo base viene impostato automaticamente quando digiti cifre sul keypad.
- Cambiando canale, il tempo base si aggiorna al tempo del nuovo canale.
- L'offset viene mostrato sull'LCD come `+02`, `-01`, ` 00`.
- Il tempo calcolato e' sempre compreso tra 1 e 9999 decisecondi.
- Cambiando la precisione (es. da 1/3 a 1/6), il tempo viene ricalcolato con lo stesso offset ma la nuova precisione.

---

## Canali

Il timer dispone di **3 canali indipendenti** (A, B, C), ciascuno con:
- Proprio tempo memorizzato
- Proprio stato relay (abilitato/disabilitato)

### Comandi

| Tasto | Azione |
|-------|--------|
| `A`   | Seleziona canale A |
| `B`   | Seleziona canale B |
| `C`   | Seleziona canale C |
| `D`   | Reset del canale corrente (tempo a 0, F-stop reset) |

### Comportamento

- Quando cambi canale, il tempo del canale precedente viene salvato automaticamente.
- Il tempo del nuovo canale viene caricato sul display.
- Lo stato F-stop (base time e offset) viene resettato al tempo del nuovo canale.
- Le impostazioni sono salvate in EEPROM ad ogni cambio.

### Relay per canale

Ogni canale ha un relay indipendente. Utile se colleghi piu' ingranditori o vuoi disabilitare il relay per un canale specifico.

- **Tenere premuto `#`** = toggle relay del canale corrente (ON/OFF)
- L'icona `⚡` sull'LCD indica relay abilitato; spazio vuoto = disabilitato.
- Se il relay e' disabilitato, il timer funziona normalmente ma il relay non si attiva.

---

## Persistenza EEPROM

Le impostazioni vengono salvate automaticamente nella EEPROM dell'Arduino e ripristinate all'accensione.

### Dati salvati (13 bytes)

| Indirizzo | Dato | Tipo |
|-----------|------|------|
| 0 | Magic byte (0xA5) | byte |
| 1 | Luminosita display (1-10) | byte |
| 2 | Mute (0/1) | byte |
| 3 | Canale attivo (1-3) | byte |
| 4-5 | Tempo canale A | int16 |
| 6-7 | Tempo canale B | int16 |
| 8-9 | Tempo canale C | int16 |
| 10 | Relay canale A | byte |
| 11 | Relay canale B | byte |
| 12 | Relay canale C | byte |

### Quando viene salvato

- Cambio canale (A, B, C)
- Modifica tempo (cifre, `*`/`#`, F-stop)
- Reset canale (D)
- Toggle mute (tenere premuto `*`)
- Toggle relay (tenere premuto `#`)
- Cambio luminosita (in FOCUS: `*`/`#`)

### Note tecniche

- Usa `eeprom_update_*` che scrive SOLO se il valore e' diverso da quello in memoria (preserva la vita della EEPROM, ~100.000 cicli di scrittura per cella).
- Al primo avvio (EEPROM vergine), il magic byte non sara' presente e verranno usati i valori di default.

---

## Suono

### Toni

| Evento | Frequenza | Durata |
|--------|-----------|--------|
| Metronomo (countdown) | 300 Hz | 80ms, ogni secondo |
| Beep avviso (test strip) | 600 Hz | 50ms, 3 decisecondi prima dello step |
| Beep step (test strip) | 600 Hz | 150ms, al raggiungimento dello step |
| Suono di stop (fine countdown) | 300 Hz | sequenza: 75ms, pausa, 75ms, pausa, 150ms |
| Conferma unmute | 600 Hz | 200ms |

### Controllo mute

- **Tenere premuto `*`** = toggle mute/unmute
- `♪` sull'LCD = suono attivo
- Spazio vuoto = mute

### Sistema non-bloccante

Il suono di stop usa una state machine (`SoundPlayer`) che esegue la sequenza di beep senza bloccare il loop principale. La funzione `tickSound()` viene chiamata ad ogni iterazione del loop.

---

## Architettura Software

### Struttura file

```
src/main.cpp                 -> setup() + loop()
include/
  types.h                    -> Enumerazioni, struct Operation/OperationStatus
  constants.h                -> Pin, toni, testi, layout keypad
  variables.h                -> Globali, oggetti hardware, Timer<3>
  functions/
    base.h                   -> relayStart(), noop(), isInError()
    config.h                 -> Inizializzazione hardware, resetTime()
    fstop.h                  -> FStopState: calcolo F-stop
    storage.h                -> EEPROM: loadSettings(), saveSettings()
    presentation.h           -> LCD e 7-segmenti: showTime(), showChannel(), ecc.
    sound.h                  -> SoundPlayer non-bloccante, emitSound(), tickSound()
    timer_manager.h          -> Timer callbacks: focus, countdown, test strip
    actions.h                -> focus(), countdown(), testStrip()
    operations.h             -> initializeOperations(), setOperationStatus()
    keypad_mgmt.h            -> evaluateKey(), switchToChannel(), F-stop nav
    encoder_mgmt.h           -> evaluateEncoderMod(), evaluateEncoderPrec()
    running_mgmt.h           -> manageMainButtonAction(), configureMainButton()
```

### Loop principale

```
loop() {
  mainButton.tick()        -> Gestione pulsante (OneButton, debounce)
  appTimer.tick()          -> Esecuzione callback timer (arduino-timer)
  tickSound()              -> State machine suono non-bloccante
  manageMainButtonAction() -> Se premuto: esegui azione. Se fermo: leggi input.
}
```

### Flusso quando il timer e' FERMO

```
manageMainButtonAction()
  -> onStop()
     -> stopAllTimers()
     -> evaluateEncoderMod()    (legge encoder modalita)
     -> evaluateEncoderPrec()   (legge encoder precisione)
     -> evaluateButtons()       (legge pulsanti encoder)
     -> evaluateKey()           (legge keypad)
```

### Flusso quando il timer e' IN ESECUZIONE

```
manageMainButtonAction()
  -> manageStarted()
     -> operationStatus.current.startedAction()
        -> focus() / countdown() / testStrip()
           -> startFocusTimer() / startCountdownTimer() / startTestStripTimer()
              -> appTimer.every(100, callback)
```

I callback del timer vengono eseguiti ogni 100ms da `appTimer.tick()`.

### Librerie utilizzate

| Libreria | Versione | Scopo |
|----------|----------|-------|
| Encoder | 1.4.4 | Lettura encoder rotativi |
| Keypad | 3.1.1 | Keypad 4x4 a matrice |
| LiquidCrystal_I2C | 1.1.4 | LCD I2C (non usato direttamente, dipendenza) |
| EncoderButton | 1.0.6 | Gestione pulsanti encoder con debounce |
| ArduinoSTL | 1.3.3 | Supporto STL minimale per String |
| OneButton | 2.6.1 | Pulsante principale con click/hold detection |
| arduino-timer | 3.0.1 | Timer non-bloccante con callback |
| WaveShare-Lib | locale | Driver LCD RGB Waveshare |

---

## Riferimento Rapido Comandi

### Tasti numerici (0-9)
Disponibili solo in COUNTDOWN e TEST STRIP. Inserimento sequenziale fino a 4 cifre.

### Tasti lettera

| Tasto | Pressione breve | Tenuta prolungata |
|-------|-----------------|-------------------|
| `A` | Seleziona canale A | - |
| `B` | Seleziona canale B | - |
| `C` | Seleziona canale C | - |
| `D` | Reset canale corrente | - |

### Tasti speciali

| Tasto | In FOCUS | In COUNTDOWN / TEST STRIP | Tenuta prolungata |
|-------|----------|---------------------------|-------------------|
| `*` | Luminosita -1 | Tempo -1 step (Secondi) / F-stop -1 (F-Stop) | Toggle Mute |
| `#` | Luminosita +1 | Tempo +1 step (Secondi) / F-stop +1 (F-Stop) | Toggle Relay canale |

### Encoder

| Encoder | Rotazione | Pressione |
|---------|-----------|-----------|
| Modalita (SX) | Cambia: FOCUS / COUNTDOWN / TEST STRIP | Toggle CO/PP (solo Test Strip) |
| Precisione (DX) | Cambia livello precisione (11 valori) | Toggle tipo: Secondi (S) / F-Stop (F) |

### Pulsante principale

| Stato | Azione |
|-------|--------|
| Timer fermo | Avvia l'operazione corrente |
| Timer in esecuzione | Ferma l'operazione |

---

## Risorse memoria

| Risorsa | Uso | Disponibile | Percentuale |
|---------|-----|-------------|-------------|
| RAM | 2050 bytes | 8192 bytes | 25.0% |
| Flash | 28082 bytes | 253952 bytes | 11.1% |
