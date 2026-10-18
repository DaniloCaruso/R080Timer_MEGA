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

Firmware C++ su **Arduino Mega 2560** con **PlatformIO** (env `megaatmega2560`). Non generare codice MicroPython su questo branch: quello vive su `feature/esp32-micropython-firmware`.

Il firmware è in riscrittura (feature `firmware-rewrite`): requisiti in `docs/firmware-rewrite/requirements-docs/EARS.it.md`, modifiche in `TECHNICAL.it.md`, decisioni in `docs/firmware-rewrite/decisions.md`. Il firmware precedente (`include/`, `lib/WaveShare-Lib/`) compila ancora finché il nuovo `src/main.cpp` non lo sostituisce; resta consultabile al tag `pre-rewrite`. Non estenderlo.

### Comandi

```
pio run                      # compila per il Mega (env megaatmega2560)
pio test -e native           # test della libreria di dominio sul PC
pio run -t upload            # compila e carica sulla scheda
pio device monitor           # monitor seriale
```

La CI (`.github/workflows/firmware.yml`) esegue compilazione Mega e test native a ogni push.

### Invarianti

- **Tre strati**:
  - `lib/r080_core/`: libreria di dominio in C++ puro, testata sul PC; parla con l'hardware solo tramite interfacce astratte.
  - `src/drivers/`: driver Arduino sottili che implementano quelle interfacce.
  - `src/main.cpp`: collega driver e dominio e fa girare il loop.
- **Dominio senza Arduino**: nessun file in `lib/r080_core/` include `Arduino.h`, `avr/*`, `util/*` o librerie Arduino. La CI lo verifica.
- **Niente blocchi nel loop**: nessun `delay()` né attesa attiva dopo l'avvio; ogni driver lavora a passi brevi.
- **ISR minima**: la ISR di Timer1 fa solo somme, confronti e scritture su `PORTL`, senza divisioni né chiamate a funzioni non inline.
- **Stato condiviso**: ogni lettura o scrittura dal main di stato condiviso con una ISR, se più larga di 8 bit, sta in `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`. Anche le scritture su `PORTL` dal main, perché `tone()` scrive sullo stesso registro.
- **Unità di tempo**: millisecondi `uint32_t` all'interno; il decimo di secondo è l'unità di interfaccia, range `1..9999` (`lib/r080_core/Time.h`).
- **EEPROM**: scrivere con `eeprom_update_*`, mai `eeprom_write_*`.
- **RAM**: niente `String`, niente ArduinoSTL, niente allocazione dinamica (`new`/`malloc`); i testi costanti stanno in `PROGMEM`; poche interfacce virtuali (le vtable stanno in RAM).
- **Librerie**: solo `Encoder`, `Keypad` e `OneButton`; l'LCD ha un driver nostro senza `delay`.
- **Test**: TDD sulle due seam del TECHNICAL (dominio intero con driver finti, `TimingCore` da solo); niente finti dei moduli interni.

---

## Agent skills

### Issue tracker

Le issue vivono in GitHub Issues (`DaniloCaruso/R080Timer_MEGA`), gestite con la CLI `gh`. Ogni issue porta l'etichetta della piattaforma: su questo branch usare `arduino`. Vedere `docs/agents/issue-tracker.md`.

### Triage labels

Etichette predefinite: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. Vedere `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `GLOSSARY.md` alla radice. Niente ADR: le decisioni di ogni feature vanno in `docs/<nome-feature>/decisions.md`. Vedere `docs/agents/domain.md`.
