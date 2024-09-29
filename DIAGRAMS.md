# DIAGRAMS — R080 Timer MEGA

Schemi Mermaid estratti direttamente dal codice (`include/constants.h`, `include/variables.h`, `include/functions/*.h`, `src/main.cpp`).

> Per visualizzare i diagrammi: aprire questo file in un viewer Markdown con supporto Mermaid (VS Code + estensione *Markdown Preview Mermaid Support*, GitHub, GitLab, Obsidian, ecc.).

---

## 1. Collegamenti hardware (vista a stella)

```mermaid
graph LR
    subgraph EXT_INPUT["Input"]
        BTN["Pulsante MAIN<br/>INPUT_PULLUP<br/>active-low"]
        ENC_M["Encoder MODE"]
        ENC_P["Encoder PRECISION"]
        KP["Keypad 4×4<br/>1 2 3 A<br/>4 5 6 B<br/>7 8 9 C<br/>* 0 # D"]
    end

    subgraph MCU["Arduino Mega 2560<br/>ATmega2560 @ 16 MHz"]
        CORE(("ATmega2560"))
    end

    subgraph EXT_OUTPUT["Output"]
        LCD["LCD Waveshare RGB 16×2<br/>I²C 0x3E + 0x60<br/>backlight ROSSO"]
        SEG["7-seg display<br/>SparkFun-style<br/>SoftwareSerial 9600"]
        RELAY["Relè ingranditore<br/>PORTL bit PL2"]
        BUZ["Buzzer<br/>tone PWM<br/>600 / 300 Hz"]
    end

    subgraph EXT_DEBUG["Debug"]
        USB["USB ↔ PC<br/>Serial 9600"]
    end

    BTN -- "D2" --> CORE
    ENC_M -- "DT=D3<br/>CLK=D4<br/>SW=D17" --> CORE
    ENC_P -- "DT=D18<br/>CLK=D19<br/>SW=D16" --> CORE
    KP -- "righe: D33 D31 D29 D27<br/>colonne: D41 D39 D37 D35" --> CORE

    CORE -- "SDA=D20<br/>SCL=D21" --> LCD
    CORE -- "A2 (TX SoftSer)" --> SEG
    CORE -- "D47<br/>(SBI/CBI su PORTL.PL2)" --> RELAY
    CORE -- "D49<br/>(tone PWM)" --> BUZ

    CORE <-- "D0/RX0 ↔ D1/TX0" --> USB

    style CORE fill:#f9c74f,stroke:#333,stroke-width:2px
    style RELAY fill:#f94144,color:#fff
    style BUZ fill:#f3722c,color:#fff
    style LCD fill:#43aa8b,color:#fff
    style SEG fill:#577590,color:#fff
```

---

## 2. Risorse interne MCU

```mermaid
graph TD
    MCU["ATmega2560<br/>(16 MHz)"]

    MCU --> T1["Timer1 16-bit<br/>CTC, OCR1A=249<br/>prescaler 64<br/>ISR a 1 kHz"]
    MCU --> T0["Timer0<br/>millis() + tone()"]
    MCU --> EE["EEPROM 4 KB<br/>13 byte usati<br/>magic 0xA5"]
    MCU --> TWI["TWI / I²C<br/>SDA=D20, SCL=D21"]
    MCU --> U0["USART0<br/>9600 baud<br/>banner debug"]
    MCU --> PL["PORTL bit PL2<br/>= pin D47<br/>relè"]

    T1 -- "ogni 1 ms" --> ISR["ISR(TIMER1_COMPA_vect)<br/>g_tickMs++"]
    ISR -- "case 2 COUNTDOWN" --> CD["g_remainingMs--<br/>== 0 → RELAY_OFF()<br/>% 1000 → metronome"]
    ISR -- "case 3 TEST_STRIP" --> TS["g_elapsedMs++<br/>≥ target → step flag"]
    ISR -- "ogni 100 ISR" --> R100["g_tickFlag100ms = 1<br/>(rendering 10 Hz)"]

    style T1 fill:#f9c74f
    style ISR fill:#f9844a,color:#fff
    style PL fill:#f94144,color:#fff
```

---

## 3. Main loop — flusso `loop()` + ISR

```mermaid
flowchart TD
    START(["loop() iteration"])
    START --> TICK["mainButton.tick()<br/>OneButton debounce"]
    TICK --> ATOM{{"ATOMIC_BLOCK<br/>snapshot &amp; clear<br/>flag ISR"}}

    ATOM --> FE{"flagEnd?"}
    FE -- "yes" --> OCE["onCountdownEnd()<br/>emitStopSound<br/>timeSet = appoTimeSet<br/>showTime(0)"]
    FE -- "no" --> FS

    OCE --> FS{"flagStep?"}
    FS -- "yes" --> OTS["onTestStripStep()<br/>TONE_UP 150ms<br/>recalcola target<br/>STEP_BY_STEP → RELAY_OFF"]
    FS -- "no" --> FM

    OTS --> FM{"flagMetro?"}
    FM -- "yes" --> OMB["onMetronomeBeat()<br/>TONE_DOWN 80ms"]
    FM -- "no" --> F100

    OMB --> F100{"flag100ms?"}
    F100 -- "yes" --> OT100["onTick100ms()<br/>render LCD + 7-seg"]
    F100 -- "no" --> SND

    OT100 --> SND["tickSound()<br/>state machine non-bloccante"]
    SND --> MGR["manageMainButtonAction()"]

    MGR --> ISP{"isMainBtnPressed?"}
    ISP -- "false" --> STOP_BR["onStop()<br/>encoder + keypad<br/>evaluateButtons"]
    ISP -- "true" --> START_BR["manageStarted()<br/>startedAction()<br/>arm Timer1 ISR"]

    STOP_BR --> END(["fine iterazione"])
    START_BR --> END
    END --> START

    %% ISR parallela
    subgraph ISR_LANE["Timer1 ISR @ 1 kHz (parallela)"]
        direction LR
        ISR_TICK["g_tickMs++"] --> ISR_SW{"g_modeActive"}
        ISR_SW -- "2" --> ISR_CD["g_remainingMs--<br/>== 0: RELAY_OFF()<br/>g_endOfCountdown=1<br/>% 1000: g_metronomeFlag=1"]
        ISR_SW -- "3" --> ISR_TS["g_elapsedMs++<br/>≥ target:<br/>g_nextStepReached=1"]
        ISR_TICK --> ISR_100["ogni 100 tick:<br/>g_tickFlag100ms=1"]
    end

    ISR_LANE -. "alza flag (volatile)" .-> ATOM

    style ISR_LANE fill:#fff3b0,stroke:#f9844a
    style ATOM fill:#f9c74f
    style START_BR fill:#90be6d,color:#fff
    style STOP_BR fill:#43aa8b,color:#fff
```

---

## 4. Stati delle modalità operative

```mermaid
stateDiagram-v2
    direction LR
    [*] --> IDLE: setup() / setupTimer1()

    IDLE --> FOCUS: mode encoder → FOCUS<br/>(g_modeActive non usato)
    IDLE --> COUNTDOWN: mode encoder → COUNTDOWN
    IDLE --> TEST_STRIP: mode encoder → TEST_STRIP

    state FOCUS {
        [*] --> F_STOPPED
        F_STOPPED --> F_RUNNING: main btn click<br/>relayStart(true)<br/>g_modeActive=1
        F_RUNNING --> F_STOPPED: main btn click<br/>stopAllTimers()
        F_RUNNING --> F_RUNNING: onTick100ms()<br/>timerCurrentValue++
    }

    state COUNTDOWN {
        [*] --> C_STOPPED
        C_STOPPED --> C_RUNNING: main btn click<br/>g_remainingMs = timeSet*100<br/>g_modeActive=2<br/>relay ON
        C_RUNNING --> C_STOPPED: main btn click<br/>stopAllTimers()
        C_RUNNING --> C_END: ISR: g_remainingMs==0<br/>RELAY_OFF (in ISR)<br/>g_endOfCountdown=1
        C_END --> C_STOPPED: onCountdownEnd()<br/>timeSet = appoTimeSet<br/>(repeat-last-exposure)
        C_RUNNING --> C_RUNNING: ISR: % 1000<br/>g_metronomeFlag=1<br/>→ TONE_DOWN 80ms
    }

    state TEST_STRIP {
        direction TB
        [*] --> T_STOPPED
        T_STOPPED --> T_RUNNING: main btn click<br/>g_modeActive=3
        T_RUNNING --> T_STEP: ISR: g_elapsedMs ≥ target<br/>g_nextStepReached=1
        T_STEP --> T_RUNNING: CONTINOUS<br/>nuovo target + TONE_UP
        T_STEP --> T_PAUSED: STEP_BY_STEP<br/>RELAY_OFF inline<br/>g_modeActive=0
        T_PAUSED --> T_RUNNING: main btn click<br/>g_modeActive=3
        T_RUNNING --> T_STOPPED: main btn click (durante run)
    }

    FOCUS --> IDLE: switch mode
    COUNTDOWN --> IDLE: switch mode
    TEST_STRIP --> IDLE: switch mode
```

---

## 5. Mappa dati EEPROM (13 byte, magic `0xA5`)

```mermaid
graph LR
    subgraph EEPROM["EEPROM (4 KB) — solo 13 byte usati"]
        direction TB
        B0["addr 0<br/>0xA5 magic"]
        B1["addr 1<br/>brightness 1..10"]
        B2["addr 2<br/>mute 0/1"]
        B3["addr 3<br/>channel 1..3"]
        B45["addr 4..5<br/>channelTime[0] int16"]
        B67["addr 6..7<br/>channelTime[1]"]
        B89["addr 8..9<br/>channelTime[2]"]
        B10["addr 10<br/>relayActivated[0]"]
        B11["addr 11<br/>relayActivated[1]"]
        B12["addr 12<br/>relayActivated[2]"]
    end

    SAVE["saveSettings()<br/>eeprom_update_*<br/>(scrive solo se cambiato)"] --> EEPROM
    LOAD["loadSettings()<br/>check magic == 0xA5"] --> EEPROM

    EEPROM --> RAM["RAM globals<br/>(variables.h)"]

    style B0 fill:#f94144,color:#fff
    style SAVE fill:#90be6d
    style LOAD fill:#43aa8b,color:#fff
```

---

## 6. Pipeline F-Stop (precisione e calcolo tempo)

```mermaid
flowchart LR
    INPUT["Input utente<br/>(keypad digit / */# / encoder)"] --> SETBASE["fstopState.setBase(timeSet)"]
    SETBASE --> STATE["FStopState<br/>{baseTime, stopOffset}"]

    NAV["Tasto * / # in FSTOP"] --> STEPUD["fstopState.stepUp() /<br/>stepDown()<br/>(stopOffset ± 1)"]
    STEPUD --> STATE

    PREC["Encoder precision<br/>(in FSTOP)"] --> PRECVAL["precisionValue<br/>(1, 2, 3, ..., 64)"]
    PRECVAL --> CALC

    STATE --> CALC["getCalculatedTime(precisionValue)<br/>= round(baseTime × 2^(stopOffset/precisionValue))<br/>clamp 1..9999"]

    CALC --> UPDATE["updateTimeFromFstop()<br/>→ timeSet"]
    UPDATE --> RENDER["showTime() + showFstopOffset()<br/>LCD row 1 cols 5..7<br/>(+02 / -01 /  00)"]

    style CALC fill:#f9c74f
    style STATE fill:#577590,color:#fff
    style RENDER fill:#43aa8b,color:#fff
```

---

## 7. Layout LCD 16×2 — slot di rendering

```mermaid
graph TD
    LCD["LCD 16×2"]

    subgraph ROW0["Riga 0 (16 colonne)"]
        direction LR
        R0_MODE["col 0..10<br/>nome modalità<br/>(FOCUS / COUNTDOWN / TEST STRIP)"]
        R0_PREC["col 12<br/>S / F<br/>(SECONDS / FSTOP)"]
        R0_MOD["col 14..15<br/>CO / PP<br/>(solo TEST STRIP)"]
    end

    subgraph ROW1["Riga 1 (16 colonne)"]
        direction LR
        R1_PR["col 0..7<br/>Pr:&lt;precisione&gt;"]
        R1_FS["col 5..7<br/>F-stop offset<br/>(+02 / -01 /  00)"]
        R1_CH["col 9..12<br/>Ch:A / Ch:B / Ch:C"]
        R1_SND["col 14<br/>♪ (glyph 1)"]
        R1_REL["col 15<br/>⚡ (glyph 2)"]
    end

    LCD --> ROW0
    LCD --> ROW1

    style R0_MODE fill:#f9c74f
    style R1_FS fill:#f3722c,color:#fff
    style R1_REL fill:#f94144,color:#fff
    style R1_SND fill:#43aa8b,color:#fff
```

---

## 8. Frequenze audio e sequenze

```mermaid
graph LR
    SND["SoundPlayer<br/>(non-bloccante)"]

    SND --> TUP["TONE_UP = 600 Hz"]
    SND --> TDN["TONE_DOWN = 300 Hz"]
    SND --> STOPSEQ["STOP_SEQUENCE<br/>(stop / fine countdown)"]

    TUP --> EV1["confirm 200ms (unmute)"]
    TUP --> EV2["test-strip step beep 150ms"]
    TUP --> EV3["test-strip warning beep 50ms<br/>(decis pre-target ∈ 1..3)"]

    TDN --> EV4["metronome 80ms<br/>(ogni 1000ms in COUNTDOWN)"]

    STOPSEQ --> SEQ["TONE_DOWN 75ms<br/>+ 150ms pausa<br/>+ TONE_DOWN 75ms<br/>+ 150ms pausa<br/>+ TONE_DOWN 150ms"]

    style TUP fill:#43aa8b,color:#fff
    style TDN fill:#f9844a,color:#fff
    style STOPSEQ fill:#f94144,color:#fff
```
