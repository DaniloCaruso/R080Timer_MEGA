# Management — Refactoring Timing R080 Timer MEGA

**Documento:** Sommario esecutivo per decisione go/no-go
**Data:** 2026-04-18
**Audience:** project owner / decisore
**Tempo di lettura stimato:** 5 minuti

---

## Executive summary

Il firmware R080 Timer MEGA attualmente funziona correttamente in produzione, ma utilizza un motore temporale basato su **polling software** (libreria `arduino-timer`) che introduce un jitter di temporizzazione proporzionale al carico del loop principale — tipicamente 2-8 ms per tick nei momenti peggiori (es. durante la scrittura al display 7-segmenti).

Si propone un **rifacimento mirato** del solo motore temporale, basato su **interrupt hardware del microcontrollore (Timer1 CTC ISR)**, che porta la precisione garantita a **±1 ms** — un miglioramento di circa 10× sul caso peggiore, e di ordini di grandezza sul bordo di fine esposizione fotografica (microsecondi).

Nello stesso intervento si procede alla **rimozione di due librerie** (`arduino-timer`, `ArduinoSTL`) che non sono più necessarie, liberando il **25% della RAM attualmente utilizzata** (da 2050 B a ~1600 B) e riducendo l'impronta Flash.

**Richiesta decisione:** approvare lo scope e procedere alla produzione del piano di implementazione.

---

## 1. Perché — motivazione

| Fattore | Situazione attuale | Post-intervento |
|---|---|---|
| Precisione fine esposizione | ±5-8 ms (worst case) | ±1 ms garantito |
| Jitter metronomo udibile | Sì, rilevabile in registrazione | No, sotto soglia 1 ms |
| Latenza reazione step test-strip | Fino a 99 ms | < 1 ms |
| RAM occupata | 25.0% (2050 / 8192 B) | ~18-20% (1400-1600 B) |
| Librerie esterne critiche | 7 | 5 |

La richiesta è **preventiva**: non vi è ad oggi un incidente di produzione documentato, ma l'architettura basata su polling non offre garanzie formali di timing, e il margine esistente può esaurirsi se il firmware verrà esteso in futuro con nuove funzionalità (display più ricchi, protocolli di comunicazione, logging).

Il lavoro è anche un **investimento in quality gate**: dopo l'intervento, nuove funzionalità possono essere aggiunte senza preoccupazione di impatto sul timing, perché il percorso critico è isolato in un'ISR.

---

## 2. Cosa — scope e non-scope

### In scope

- Sostituzione motore temporale: da `arduino-timer` (software polling) a **Timer1 CTC ISR (hardware interrupt)** a 1 kHz
- Commutazione del relè ingranditore dentro l'ISR per massima precisione sul bordo di fine esposizione
- Rebase della state machine audio (`SoundPlayer`) sul clock hardware unificato
- Rimozione della libreria `ArduinoSTL` e sostituzione della classe `String` con stringhe nativo-C
- Preservazione integrale della semantica utente e del layout EEPROM

### Fuori scope (volontariamente)

- Librerie input/output mantenute inalterate (`Encoder`, `Keypad`, `OneButton`, `EncoderButton`, `LiquidCrystal_I2C`)
- Nessuna modifica al pinout hardware
- Nessuna modifica al layout EEPROM o alla sequenza di boot
- Nessuna modifica alla UI (stringhe, comportamenti tastiera, layout LCD)
- Nessuna ristrutturazione architetturale (il modello one-TU header-everything resta)

### Fuori scope ma valutabile come fase successiva

- Migrazione del display 7-segmenti da `SoftwareSerial` a una seriale hardware (Serial3) — richiede lo spostamento di un filo. Elimina 5 ms di blocco ISR residuo, utile solo se si dovesse puntare a ±100 µs in futuro.

---

## 3. Come — approccio e struttura del lavoro

Il rifacimento è organizzato in **6 fasi**, ciascuna corrispondente a un commit autonomo, ciascuna produttiva di un sistema funzionante. Nessuna fase richiede un "big bang" di integrazione.

| # | Fase | Contenuto | Verifica |
|---|---|---|---|
| A | Motore temporale | Timer1 ISR + flag; rimozione `arduino-timer` | Cronometro su countdown |
| B | Relè nell'ISR | Commutazione pin diretta in ISR | Esposizione 10 s con cronometro esterno |
| C | Audio coerente | `SoundPlayer` su clock unificato | Ascolto metronomo |
| D | Precisione beep | Metronomo e step beep trigger ISR | Analisi audio con std-dev < 1 ms |
| E | Rimozione librerie | `String`/`ArduinoSTL` → `const char*`/PROGMEM | Confronto dimensioni build |
| F | Validazione | Test manuale completo + rilettura DOCS.md | Zero regressioni funzionali |

Tutto il lavoro avviene su un branch dedicato (`Performance_Rewrite`). Il branch di produzione (`Libraries_Refactoring`) rimane intatto per tutta la durata e funge da fallback assoluto.

---

## 4. Stima di effort

| Fase | Effort tecnico | Commenti |
|---|---|---|
| A | Medio | Configurazione registri Timer1, ISR minima, porting 3 modi sul flag pattern |
| B | Basso | Una riga nell'ISR, una costante `PORTL`/`PL2` |
| C | Basso | Sostituzione `millis()` → `g_tickMs` in 5-10 punti |
| D | Basso | Due flag aggiuntivi nell'ISR, due handler nel main |
| E | Medio-alto | Tocca ~15 file, richiede compilazione iterativa per catch |
| F | Medio | Test manuali, misure strumentali |

**Stima totale:** 2-3 giorni di lavoro tecnico concentrato, o 1 settimana a ritmo sostenibile.

Il lavoro non richiede hardware aggiuntivo oltre a un cronometro e un dispositivo per registrare audio (smartphone è sufficiente per le misure di std-dev del metronomo).

---

## 5. Rischi e gestione

| Rischio | Probabilità | Impatto | Mitigazione |
|---|---|---|---|
| Contesa Timer1 con codice Arduino core | Trascurabile | Alto | Verifica preventiva in fase A con LED blink; Timer0/Timer2 confermati liberi |
| Degrado del metronomo per blocchi SoftwareSerial | Media | Medio | Scheduling display fuori dalla finestra critica; mitigazione residua valutabile con fase G opzionale |
| Race condition su variabili condivise | Media | Medio | Pattern `ATOMIC_BLOCK` standard, applicato sistematicamente |
| Rimozione `ArduinoSTL` rompe dipendenze transitive | Bassa | Basso | Rollback della sola fase E; metà del guadagno RAM resta |
| Regressione funzionale non rilevata | Bassa | Alto | Test manuale esplicito di tutti i comportamenti in `DOCS.md` in fase F |

**Rischio complessivo del progetto: basso.** L'intervento è localizzato, incrementale, completamente reversibile. Il path di rollback è semplice (`git reset` o abbandono del branch).

---

## 6. Criteri di successo (go/no-go finale)

Il rifacimento è dichiarato completato solo se **tutti** i seguenti criteri sono soddisfatti:

1. ✅ **Precisione esposizione:** errore massimo 2 ms su esposizioni fino a 60 s misurato con cronometro esterno
2. ✅ **Metronomo stabile:** deviazione standard del periodo tick < 1 ms su 30 s di registrazione
3. ✅ **RAM:** occupazione ≤ 20% (1639 / 8192 B)
4. ✅ **Flash:** occupazione ≤ 15% (38093 / 253952 B)
5. ✅ **Zero regressioni:** ogni comportamento utente descritto in `DOCS.md` produce lo stesso risultato del branch `Libraries_Refactoring`
6. ✅ **Build pulita:** `pio run` senza warning aggiuntivi, nessun file `.cpp` aggiunto, singola unità di traduzione preservata

---

## 7. Decisione richiesta

**Approvare** lo scope descritto e autorizzare:
- la creazione del branch `Performance_Rewrite`
- la produzione del piano di implementazione dettagliato (task list eseguibile) sulla base di `Technical.md`
- l'esecuzione incrementale delle 6 fasi con verifica after-each

**Oppure richiedere** modifiche allo scope (restringimento, ampliamento, riorganizzazione delle fasi) prima di procedere.

---

## 8. Documenti correlati

- `EARS.md` — requisiti formali (sorgente di verità per la verifica fase F)
- `Analisys.md` — sintesi del processo di brainstorming e delle opzioni considerate
- `Technical.md` — dettaglio tecnico di implementazione (codice, registri, diff per file)
- `CLAUDE.md` (radice progetto) — documentazione architetturale esistente
- `DOCS.md` / `DOCS-IT.md` — specifica comportamento utente (autorità per verifica non-regressione)
