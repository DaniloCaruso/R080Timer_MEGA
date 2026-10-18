# CLAUDE.md

Guida per agenti Claude Code. Per hardware e comportamento utente, vedere `README.md`.

---

## 1. Cos'è questo progetto

**R080 Timer** — firmware per timer da camera oscura (ingranditore fotografico). 3 modalità (Focus, Countdown, Test Strip), navigazione F-Stop, 3 canali con memoria, persistenza impostazioni, suoni non bloccanti.

Lo sviluppatore è italiano: stringhe UI e commenti in parte in italiano. Preservarli. **Rispondere all'utente in italiano.**

---

## 2. Branch

`main` contiene il timer Arduino originale. Da `main` partono due linee di sviluppo indipendenti, ognuna con il proprio `CLAUDE.md` che estende questo:

| Branch | Piattaforma | Etichetta issue |
|---|---|---|
| `feature/esp32-micropython-firmware` | MicroPython su ESP32-WROOM-32 | `esp32` |
| `feature/arduino-mega-firmware` | C++/PlatformIO su Arduino Mega 2560 | `arduino` |

Le modifiche comuni (documentazione, configurazione agenti) vanno su `main`; i due branch vengono poi ribasati su `main`.

---

## 3. Questo branch: Arduino Mega 2560

Firmware C++ su **Arduino Mega 2560** con **PlatformIO** (env `megaatmega2560`). Non generare codice MicroPython su questo branch: quello vive su `feature/micropython-esp32-port`.

Riferimenti: `TECNICO.md` (architettura completa), `PINOUT.md` (pin e risorse AVR), `DIAGRAMS.md` (flussi e state machine).

### Comandi

```
pio run                      # compila
pio run -t upload            # compila e carica sulla scheda
pio device monitor           # monitor seriale
```

### Invarianti

- **Single Translation Unit**: gli header in `include/functions/*.h` definiscono le funzioni e sono inclusi da `src/main.cpp` in un ordine preciso. `timer_isr.h` deve stare prima di `base.h`. Non riordinare gli `#include`.
- **Motore temporale**: Timer1 ISR a 1 kHz (`timer_isr.h`). Le variabili condivise ISR ↔ main multi-byte si leggono e scrivono in `ATOMIC_BLOCK`.
- **Unità di tempo**: decisecondi (1/10 s), range `0..9999`.
- **Canali**: `channel` vale 1..3, gli array sono 0-indexed (`[channel - 1]`).
- **EEPROM**: scrivere con `eeprom_update_*`, mai `eeprom_write_*`.
- **RAM**: niente `String` né ArduinoSTL; i testi stanno in `PROGMEM`.

---

## Agent skills

### Issue tracker

Le issue vivono in GitHub Issues (`DaniloCaruso/R080Timer_MEGA`), gestite con la CLI `gh`. Ogni issue porta l'etichetta della piattaforma: su questo branch usare `arduino`. Vedere `docs/agents/issue-tracker.md`.

### Triage labels

Etichette predefinite: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. Vedere `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `GLOSSARY.md` alla radice. Niente ADR: le decisioni di ogni feature vanno in `docs/<nome-feature>/decisions.md`. Vedere `docs/agents/domain.md`.
