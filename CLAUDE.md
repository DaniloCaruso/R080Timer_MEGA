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

## Agent skills

### Issue tracker

Le issue vivono in GitHub Issues (`DaniloCaruso/R080Timer_MEGA`), gestite con la CLI `gh`. Ogni issue porta l'etichetta della piattaforma (`esp32` / `arduino`). Vedere `docs/agents/issue-tracker.md`.

### Triage labels

Etichette predefinite: `needs-triage`, `needs-info`, `ready-for-agent`, `ready-for-human`, `wontfix`. Vedere `docs/agents/triage-labels.md`.

### Domain docs

Single-context: `GLOSSARY.md` alla radice. Niente ADR: le decisioni di ogni feature vanno in `docs/<nome-feature>/decisions.md`. Vedere `docs/agents/domain.md`.
