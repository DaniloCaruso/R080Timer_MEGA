# Decisioni — firmware-rewrite

Riscrittura enterprise-grade del firmware R080 Timer per Arduino Mega 2560. Decisioni prese nella sessione di grilling del 2026-10-18.

## Riscrittura da zero, non correzione

**Contesto:** il branch attuale compila ma ha regressioni gravi (beep di stop in loop, 7-seg a 0000 da fermo, memoria canali persa, loop idle ~110 ms) e problemi strutturali (stato globale, redraw a ogni giro, logica non testabile).
**Decisione:** riscrivere il firmware. Il codice attuale (tag `pre-rewrite`) è solo un riferimento: se ne riprendono le idee validate (Timer1 ISR, relè spento in ISR, formula F-Stop).
**Conseguenze:** nessuna compatibilità di codice con il vecchio firmware; `docs/refactoring-timing/` resta come riferimento storico.

## Piattaforma e cablaggio

**Contesto:** SoftwareSerial verso il 7-seg disabilita gli interrupt ~1 ms per byte.
**Decisione:** Arduino Mega 2560 originale, stessi componenti. Ammesse modifiche minime al cablaggio che migliorano l'affidabilità, a partire dal 7-seg su Serial3 (pin 14).
**Conseguenze:** `PINOUT.md` va aggiornato; il pin A2 si libera.

## Perimetro funzionale della v1

**Decisione:** la v1 fa esattamente le funzioni attuali: Focus, Countdown, Test Strip (continuo e passo-passo) in secondi o F-Stop, 3 canali con memoria, mute, gate relè per canale, luminosità 7-seg, persistenza. Le nuove funzioni vanno in backlog e entrano solo dopo che la base è stabile.

## Sicurezza dell'esposizione

**Decisione:** requisiti non negoziabili: relè OFF all'accensione, al reset e in ogni errore; watchdog hardware con relè OFF; brown-out detection; nessuna esposizione avviata o fermata da rimbalzi; impostazioni EEPROM validate da CRC con fallback ai default.

## Precisione e calibrazione del clock

**Contesto:** la scheda è un Arduino Mega 2560 originale.
**Decisione:** obiettivo ±1 ms sui bordi di esposizione rispetto al clock MCU, più un fattore di correzione in ppm salvato in EEPROM, misurato una volta con un riferimento esterno.

## Priorità d'uso in camera oscura

**Decisione:** (1) comandi reattivi, nessuna pressione persa; (2) luci sicure: LCD solo rosso e attenuabile, 7-seg attenuabile, attenuazione durante l'esposizione; (3) suoni chiari e distinguibili per inizio, secondi, step e fine.

## Processo di lavoro

**Decisione:** grilling → `GLOSSARY.md` + questo file → `dev-flow-docs` (EARS, TECHNICAL, MANAGEMENT in `requirements-docs/`) → PRD → issue GitHub con etichetta `arduino` → implementazione TDD. Documentazione della feature in `docs/firmware-rewrite/`.

## Branch e tag

**Decisione:** il codice prima della riscrittura è marcato con il tag `pre-rewrite`. Il branch di lavoro diventa `feature/arduino-mega-firmware`; per simmetria anche il branch ESP32 viene rinominato (`feature/esp32-micropython-firmware`, il cui `CLAUDE.md` si aggiorna su quel branch). Il nome nei file condivisi (`CLAUDE.md`, `docs/agents/issue-tracker.md`) si aggiorna su `main`, poi i due branch si ribasano. La cancellazione dei vecchi branch remoti richiede conferma esplicita.

## Test automatici e CI

**Decisione:** ambiente PlatformIO `native` con Unity per testare la logica di dominio sul PC; GitHub Action che esegue `pio run` e `pio test -e native` a ogni push; checklist di collaudo su scheda firmata dall'operatore per ogni release.

## Librerie

**Decisione:** si tengono `Encoder`, `Keypad` e `OneButton` (per tutti e 3 i pulsanti). Si eliminano `EncoderButton` e `LiquidCrystal_I2C`. Il driver LCD Waveshare viene riscritto senza `delay` per byte.

## Unità di tempo

**Decisione:** internamente millisecondi (`uint32_t`). Il decimo di secondo resta l'unità di interfaccia (display, keypad, persistenza). Tempo impostabile da 0,1 s a 999,9 s.

## Interfaccia: stessa mappa, difetti corretti

**Decisione:** stesso layout LCD e stesso significato di tasti ed encoder. Si correggono: azione al rilascio dopo un HOLD; cifre che si accodano dopo un'esposizione; 7-seg che non torna al tempo impostato a fine esposizione; test strip che non riparte da zero dopo uno stop manuale; offset F-Stop non visibile.

## Formato EEPROM nuovo

**Decisione:** layout versionato con numero di versione e CRC8. Nessuna migrazione dal vecchio formato: alla prima accensione si riparte dai default.

## Architettura a tre strati

**Contesto:** la logica deve essere testabile senza la scheda.
**Decisione:** si abbandona il modello single-TU. Tre strati: (1) dominio in C++ puro, senza Arduino, che parla con l'hardware solo tramite interfacce astratte; (2) driver Arduino sottili che implementano le interfacce (relè su PORTL, LCD I²C, 7-seg su Serial3, Timer1/ISR, watchdog, EEPROM, Keypad, Encoder, OneButton); (3) `main.cpp` che collega driver e dominio. La logica della ISR sta in una funzione pura chiamata dalla ISR.
**Conseguenze:** gli invarianti di `CLAUDE.md` (single TU, ordine degli include) vanno riscritti.

## Luci in camera oscura

**Decisione:** LCD sempre e solo rosso, in ogni stato. Luminosità di LCD e 7-seg separate e persistite. Durante l'esposizione l'LCD scende a una luminosità bassa (non spento) e torna normale alla fine; il 7-seg resta alla luminosità scelta.

## Suoni

**Decisione:** avvio = beep acuto breve; metronomo = tick grave a ogni secondo dall'inizio dell'esposizione; step test strip = beep acuto; preavviso step = 3 tick negli ultimi 0,3 s; fine = 3 toni discendenti. Il mute silenzia tutto tranne il suono di fine esposizione.

## Pulsante principale durante il countdown

**Decisione:** premere durante l'esposizione la annulla (relè OFF, suono di fine); la pressione successiva riparte dal tempo pieno. Pausa e ripresa vanno nel backlog.

## Spegnimento automatico in Focus

**Decisione:** in Focus la lampada si spegne da sola dopo un tempo massimo, con un avviso sonoro 10 s prima. Il tempo massimo si sceglie dal menu di servizio tra 3, 5, 10, 15 e 20 minuti; default 5 minuti.

## Watchdog e cause di reset

**Decisione:** watchdog a 500 ms con relè OFF. Dopo un reset da watchdog o brown-out l'LCD mostra la causa per 3 s. Un contatore degli eventi è salvato in EEPROM e leggibile dal menu di servizio.

## Calibrazione del clock

**Decisione:** menu di servizio (tenere premuto `D` all'accensione): countdown di riferimento di 600,0 s, l'utente inserisce il tempo misurato con un cronometro, il firmware calcola e salva la correzione in ppm. Lo stesso menu mostra i contatori di reset e permette il ripristino dei valori di fabbrica. Il menu ospita anche le diagnostiche (vedi sotto).

## Diagnostica

**Decisione:** il menu di servizio contiene: info (versione, data build); contatori (reset per causa, numero esposizioni, ore lampada); salute del sistema (durata massima di loop e ISR, tick Timer1 persi); test hardware per relè, buzzer, 7-seg, LCD, keypad, encoder e pulsante principale; calibrazione clock e ripristino di fabbrica. In più un log eventi su USB a 115200 baud (avvio, start/stop esposizione con durata misurata, errori).

## Test strip

**Decisione:** step cumulativi T, T+P, T+2P (secondi) o T·2^(k/p) calcolati dalla base (F-Stop). Continuo: relè sempre acceso, beep a ogni step. Passo-passo: primo impulso T, poi solo l'incremento; il relè si spegne dalla ISR. Gli step proseguono fino allo stop (limite 999,9 s). Durante la prova l'LCD mostra il numero di step, il 7-seg il tempo cumulativo; in pausa passo-passo il 7-seg mostra il tempo cumulativo del prossimo step.

## Inserimento tempo da keypad

**Decisione:** cifre "a calcolatrice" da destra, massimo 4 cifre (la più vecchia esce a sinistra). Il primo numero dopo qualsiasi altra azione inizia un tempo nuovo. `D` azzera il tempo del canale. Il tempo inserito diventa la base F-Stop con offset 0. Salvataggio in EEPROM 2 s dopo l'ultima cifra o all'avvio dell'esposizione.

## Canale = impostazione di stampa completa

**Decisione:** ogni canale A/B/C memorizza tutto lo stato di lavoro: tempo, base e offset F-Stop, abilitazione relè, modalità, tipo e valore di precisione, continuo/passo-passo. Cambiare canale ripristina l'intera impostazione.
**Conseguenze:** gli encoder vanno letti come variazioni relative (non posizione assoluta), perché la modalità e la precisione correnti arrivano dal canale.

## Cambio modalità e persistenza

**Decisione:** il cambio modalità non azzera il tempo. Durante l'esposizione encoder, tastierino e cambio canale sono ignorati: funziona solo il pulsante principale. All'accensione si ritrova tutto com'era (canale attivo e relativo stato, luminosità, mute).

## Display durante il test strip

**Decisione:** a prova in corso la riga 2 dell'LCD mostra `St:n` al posto della precisione; canale, icona mute e icona relè restano al loro posto.

## Avvio con tempo zero

**Decisione:** start con tempo 0,0 in Countdown o Test Strip emette un beep di errore e il timer resta fermo.

## Pulsante principale alla pressione

**Decisione:** start e stop scattano alla pressione del pulsante (antirimbalzo ~20 ms), non al rilascio. La pressione lunga non ha funzioni.

## Liste di precisione

**Decisione:** invariate. F-Stop: 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64. Secondi: 1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20.

## Versioni e rilasci

**Decisione:** versionamento semantico a partire da `v2.0.0`; versione mostrata all'avvio e nel menu di servizio; `CHANGELOG.md`; ogni rilascio è un tag git legato alla checklist di collaudo firmata.

## Documenti esistenti

**Decisione:** a riscrittura finita si riscrivono manuale utente (`DOCS-IT.md`/`DOCS.md`), `README` e `PINOUT.md`; `TECNICO.md` e `DIAGRAMS.md` vengono sostituiti dai nuovi documenti tecnici; i PDF escono dal repository; `docs/refactoring-timing/` resta come archivio.

## Regolazione delle luminosità

**Decisione:** solo in Focus: `*`/`#` regolano la luminosità dell'LCD; tenendo premuto `0` e premendo `*`/`#` si regola il 7-seg. Il livello basso dell'LCD durante l'esposizione si imposta dal menu di servizio (default: minimo visibile).

## Pressioni lunghe

**Decisione:** HOLD di `*` (mute) e `#` (abilitazione relè del canale) dopo 1 s, confermato da un breve beep.

## Tolleranza ai guasti delle periferiche

**Decisione:** ogni comunicazione con l'LCD ha un timeout e non può bloccare. Se l'LCD non risponde il timer continua con il solo 7-seg e registra l'evento nel log. Durante un'esposizione nessuna periferica guasta può interromperla: relè e tempo non dipendono mai dal display.

## Metronomo

**Decisione:** il tick al secondo suona solo in Countdown. In Test Strip solo beep di step e preavviso; in Focus solo l'avviso di spegnimento automatico.

## Schermata di avvio

**Decisione:** `R080 Timer` + versione per 1,5 s, poi l'ultimo stato salvato. Dopo un reset anomalo la causa resta visibile per 3 s.

## Mancanza di corrente durante l'esposizione

**Contesto:** alla riaccensione la carta potrebbe essersi spostata; riprendere l'esposizione rovinerebbe la stampa.
**Decisione:** un'esposizione interrotta non viene mai ripresa: alla riaccensione il timer riparte fermo con il relè spento. La sola segnalazione dell'esposizione interrotta va nel backlog.

## Backlog (dopo la v1)

Pausa e ripresa dell'esposizione; soglia di brown-out a 4,3 V via ISP con riconoscimento del reset da calo di tensione; compensazione del dry-down; programmi salvati; pedale; controllo della luce di sicurezza; segnalazione di esposizione interrotta da mancanza di corrente.

## Brown-out e causa del reset (giro 7)

**Contesto:** il bootloader stk500v2 azzera `MCUSR`; la soglia di brown-out di fabbrica è 2,7 V e si cambia solo via ISP.
**Decisione:** nella v1 si tiene la soglia di fabbrica e si riconosce solo il reset da watchdog (marcatore in RAM). La soglia a 4,3 V via ISP (fuse esteso `0xFD` → `0xFC`) va nel backlog.

## Suoni: parametri (giro 7)

**Decisione:** avvio 1000 Hz 60 ms; metronomo 300 Hz 80 ms; step 600 Hz 150 ms; preavviso 3 × 800 Hz 30 ms; fine 600/450/300 Hz 150 ms con pause di 80 ms; errore 150 Hz 300 ms; conferma 1200 Hz 40 ms; avviso Focus 2 × 450 Hz 200 ms. Valori in un'unica tabella, ritoccabili a orecchio durante il collaudo.

## Livelli di luminosità (giro 7)

**Decisione:** 8 livelli per LCD e 8 per 7-seg, non lineari (più fitti in basso). Default: 7-seg al livello equivalente all'attuale valore 10/255, LCD livello 3, LCD in esposizione livello 1.

## Chiusura del test strip passo-passo (giro 7)

**Decisione:** in pausa tra due step, `D` chiude il test strip (7-seg al tempo base, prossima prova dal primo step). Anche A/B/C o il cambio modalità lo chiudono.

## Conteggio delle esposizioni (giro 7)

**Decisione:** contano Countdown e test strip (un test strip = una esposizione). Il Focus non è un'esposizione ma contribuisce alle ore di lampada, come ogni tempo a relè acceso.

## Pull-down sul relè (giro 7)

**Decisione:** resistenza preventiva da 10 kΩ tra D47 e GND. L'utente ritiene che il modulo relè ne abbia già una; in parallelo è comunque innocua.

## Metodo di calibrazione (giro 7)

**Decisione:** metodo principale: script `tools/calibrate.py` che confronta per 30 minuti il conteggio dei ms scritto dal firmware sul log USB con l'orologio del PC e stampa la correzione in ppm. Alternativa senza PC descritta nel manuale: registrazione audio dei beep di inizio e fine di un countdown di 600,0 s. Il menu di servizio accetta la correzione direttamente in ppm oppure il tempo misurato.

## Navigazione del menu di servizio (giro 7)

**Decisione:** encoder della modalità per scorrere, il suo pulsante per entrare o confermare, `D` per tornare indietro, tasti numerici per i valori; l'ultima voce "Esci" riavvia il timer in uso normale.
