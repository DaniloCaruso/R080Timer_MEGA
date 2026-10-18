# EARS — Requisiti Refactoring Timing R080 Timer MEGA

**Documento:** Requisiti in formato EARS (Easy Approach to Requirements Syntax)
**Progetto:** R080 Timer MEGA — rifacimento motore temporale
**Branch target:** `Performance_Rewrite` (da `Libraries_Refactoring`)
**Data:** 2026-04-18

---

## Legenda notazione EARS

| Pattern | Forma | Uso |
|---|---|---|
| **Ubiquitous** | *The system shall …* | Proprietà sempre vere |
| **Event-driven** | *When `<trigger>`, the system shall …* | Reazione a un evento |
| **State-driven** | *While `<state>`, the system shall …* | Comportamento in uno stato |
| **Unwanted** | *If `<condition>`, then the system shall …* | Gestione di condizioni anomale |
| **Optional** | *Where `<feature>`, the system shall …* | Comportamento condizionato |

---

## REQ-1 — Precisione temporale

### REQ-1.1 (Ubiquitous)
Il sistema **shall** utilizzare un Timer hardware AVR in modalità CTC con ISR a frequenza 1 kHz come sorgente primaria di temporizzazione.

### REQ-1.2 (Event-driven)
**When** il countdown in modalità COUNTDOWN raggiunge zero millisecondi residui, il sistema **shall** disattivare il pin del relè ingranditore entro 1 ms dal bordo teorico.

### REQ-1.3 (Ubiquitous)
L'errore accumulato di temporizzazione su un'esposizione di durata fino a 60 secondi **shall** essere inferiore a 2 ms.

### REQ-1.4 (Event-driven)
**When** il contatore interno in modalità TEST_STRIP raggiunge il valore target del prossimo step, il sistema **shall** emettere il beep di step entro 1 ms.

### REQ-1.5 (State-driven)
**While** la modalità COUNTDOWN è attiva e il sistema non è in mute, il sistema **shall** emettere un tick del metronomo a 300 Hz per 80 ms ogni 1000 ms, con deviazione standard del periodo inferiore a 1 ms misurata su 30 secondi di registrazione audio.

---

## REQ-2 — Comportamento ISR

### REQ-2.1 (Ubiquitous)
L'ISR di Timer1 **shall** completare l'esecuzione in meno di 20 µs nel caso peggiore.

### REQ-2.2 (Ubiquitous)
L'ISR di Timer1 **shall** eseguire esclusivamente operazioni su variabili `volatile`, accessi a registri I/O e incrementi/confronti di contatori interi.

### REQ-2.3 (Unwanted)
**If** un accesso da main loop a una variabile `volatile` condivisa di larghezza superiore a 8 bit è in corso, **then** il sistema **shall** proteggere la lettura con un blocco `ATOMIC_BLOCK(ATOMIC_RESTORESTATE)`.

### REQ-2.4 (Event-driven)
**When** il contatore `g_remainingMs` è uguale a zero durante un COUNTDOWN attivo, l'ISR **shall** commutare il pin del relè direttamente via manipolazione del registro `PORTL`, senza passare per `digitalWrite()`.

---

## REQ-3 — Preservazione funzionale

### REQ-3.1 (Ubiquitous)
Il sistema **shall** preservare la semantica utente di tutti i 3 modi operativi (FOCUS, COUNTDOWN, TEST_STRIP) come descritta in `DOCS.md`.

### REQ-3.2 (Ubiquitous)
Il sistema **shall** preservare il layout EEPROM esistente (magic byte `0xA5`, 13 byte) senza alterarne l'indirizzamento.

### REQ-3.3 (Ubiquitous)
Il sistema **shall** preservare l'unità di misura tempo "decisecondi" come rappresentazione esterna (display, keypad, EEPROM, canali).

### REQ-3.4 (Ubiquitous)
Il sistema **shall** preservare i comportamenti F-Stop documentati: `baseTime * 2^(stopOffset/precisionValue)` con clamp `1..9999`.

### REQ-3.5 (Ubiquitous)
Il sistema **shall** preservare la funzionalità "repeat-last-exposure" (`timeSet = appoTimeSet` a fine countdown).

### REQ-3.6 (Ubiquitous)
Il sistema **shall** preservare il pinout hardware corrente senza richiedere spostamento di alcun filo.

---

## REQ-4 — Risorse

### REQ-4.1 (Ubiquitous)
La memoria RAM utilizzata dopo il rifacimento **shall** essere inferiore o uguale al 20% della RAM totale disponibile (1639 / 8192 byte).

### REQ-4.2 (Ubiquitous)
La memoria Flash utilizzata dopo il rifacimento **shall** essere inferiore o uguale al 15% della Flash totale disponibile (38093 / 253952 byte).

### REQ-4.3 (Ubiquitous)
Il sistema **shall** rimuovere la dipendenza dalla libreria `mike-matera/ArduinoSTL`.

### REQ-4.4 (Ubiquitous)
Il sistema **shall** rimuovere la dipendenza dalla libreria `contrem/arduino-timer`.

### REQ-4.5 (Ubiquitous)
Il sistema **shall** sostituire l'uso della classe `String` con stringhe `const char*` (letterali o PROGMEM) in tutti i file sorgente del progetto.

---

## REQ-5 — Modello di compilazione

### REQ-5.1 (Ubiquitous)
Il sistema **shall** preservare il modello a singola unità di traduzione con `src/main.cpp` come unico file `.cpp` del progetto.

### REQ-5.2 (Ubiquitous)
Il sistema **shall** mantenere tutti i nuovi moduli come file header (`.h`) con funzioni definite inline sotto guardie `#ifndef`/`#define`/`#endif`.

### REQ-5.3 (Ubiquitous)
Il sistema **shall** preservare l'ordine di inclusione load-bearing documentato in `CLAUDE.md §5`.

---

## REQ-6 — Interazione display e audio

### REQ-6.1 (State-driven)
**While** il COUNTDOWN è attivo, il sistema **shall** aggiornare il display 7-segmenti con frequenza non superiore a 10 Hz.

### REQ-6.2 (State-driven)
**While** il COUNTDOWN è attivo, il sistema **shall** evitare di eseguire scritture bloccanti su SoftwareSerial nella finestra di ±5 ms attorno al tick del metronomo.

### REQ-6.3 (Event-driven)
**When** viene rilevata una pressione di tasto del keypad, il sistema **shall** elaborarla solo se il COUNTDOWN non è attivo (comportamento preservato).

### REQ-6.4 (Event-driven)
**When** l'utente attiva o disattiva il mute, il sistema **shall** persistere lo stato in EEPROM (comportamento preservato).

---

## REQ-7 — Build e validazione

### REQ-7.1 (Ubiquitous)
Il progetto **shall** compilare senza warning aggiuntivi rispetto alla baseline del branch `Libraries_Refactoring`.

### REQ-7.2 (Ubiquitous)
Il progetto **shall** compilare con `pio run` nell'ambiente `[env:megaatmega2560]` senza modifiche al toolchain.

### REQ-7.3 (Event-driven)
**When** una fase del piano di migrazione è completata, il commit relativo **shall** essere autonomamente funzionante (sistema bootable e operativo a livello utente).

---

## REQ-8 — Rollback

### REQ-8.1 (Unwanted)
**If** una fase del refactoring introduce regressione non risolvibile entro il ciclo corrente, **then** il sistema **shall** consentire rollback tramite `git reset --hard` al commit precedente la fase.

### REQ-8.2 (Ubiquitous)
Il branch `Libraries_Refactoring` **shall** rimanere intatto come fallback finale di produzione per tutta la durata del rifacimento.

---

## Tracciabilità

Ogni requisito è linkato alle fasi implementative (vedi `Technical.md §Fasi`):

| Requisito | Fase |
|---|---|
| REQ-1.1, REQ-1.5, REQ-2.1, REQ-2.2 | A, D |
| REQ-1.2, REQ-2.4 | B |
| REQ-1.4 | D |
| REQ-3.* | (tutte) — verifica in F |
| REQ-4.1, REQ-4.2, REQ-4.3, REQ-4.5 | E |
| REQ-4.4 | A |
| REQ-5.* | (tutte) |
| REQ-6.* | A, F |
| REQ-7.*, REQ-8.* | F |
