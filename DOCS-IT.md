# R080 Timer — Manuale d'uso (firmware v2.1.0)

[English version](DOCS.md)

R080 Timer comanda la lampada dell'ingranditore: decide per quanto tempo la luce impressiona la carta. Ha tre Modalità (Focus, Countdown, Test strip), precisione in secondi o in frazioni di stop (F-Stop), tre Canali con memoria e un Menu di servizio per diagnostica e calibrazione.

I tempi si leggono e si inseriscono in decimi di secondo, da 0,1 a 999,9 s.

## Indice

1. [Comandi](#1-comandi)
2. [Display](#2-display)
3. [Inserire il tempo](#3-inserire-il-tempo)
4. [Precisione: Secondi e F-Stop](#4-precisione-secondi-e-f-stop)
5. [Modalità](#5-modalità)
6. [Canali e memoria](#6-canali-e-memoria)
7. [Suoni e mute](#7-suoni-e-mute)
8. [Luci in camera oscura](#8-luci-in-camera-oscura)
9. [Accensione e sicurezza](#9-accensione-e-sicurezza)
10. [Menu di servizio](#10-menu-di-servizio)
11. [Calibrazione del clock](#11-calibrazione-del-clock)
12. [Log USB](#12-log-usb)
13. [Riferimento rapido](#13-riferimento-rapido)

---

## 1. Comandi

| Comando | Azione |
|---|---|
| Pulsante principale | Avvia l'Esposizione; durante l'Esposizione la annulla. Reagisce alla pressione, non al rilascio. |
| Cifre `0`–`9` | Inseriscono il tempo (vedi §3). Lo `0` entra al rilascio del tasto. |
| `*` / `#` | Diminuiscono / aumentano il tempo secondo la precisione (§4). In Focus regolano la luminosità dell'LCD (§8). |
| `0` tenuto + `*` / `#` | In Focus: luminosità del 7 segmenti (§8). |
| `*` tenuto 1 s | Mute acceso/spento; spegnendolo suona un beep di conferma (§7). |
| `#` tenuto 1 s | Abilitazione relè del Canale attivo, con un beep di conferma (§6). |
| `A` / `B` / `C` | Attivano il Canale corrispondente (§6). |
| `D` | Azzera tempo impostato, Tempo base e Offset del Canale attivo. |
| Encoder della Modalità, rotazione | Modalità successiva o precedente: Focus, Countdown, Test strip. |
| Encoder della Modalità, pressione | In Test strip: Avanzamento continuo (`CO`) o passo-passo (`PP`). |
| Encoder della precisione, rotazione | Valore di precisione successivo o precedente (11 valori, in ciclo). |
| Encoder della precisione, pressione | Tipo di precisione: Secondi o F-Stop. |

Durante un'Esposizione (Focus acceso compreso) tastierino ed encoder sono ignorati: conta solo il pulsante principale.

## 2. Display

### 7 segmenti

Mostra un tempo in secondi con un decimale, a quattro cifre: `010.0` = 10,0 s, `999.9` = massimo.

| Stato | Cosa mostra |
|---|---|
| Fermo | Il tempo impostato del Canale attivo |
| Countdown in corso | Il tempo residuo, arrotondato per eccesso |
| Focus acceso | Il tempo trascorso; oltre 999,9 s in minuti.secondi (`17.05` = 17 min 05 s) |
| Test strip in corso | Il tempo cumulativo dall'inizio della prova |
| Test strip passo-passo in pausa | Il tempo cumulativo dello Step successivo |

### LCD

Due righe da 16 caratteri, retroilluminazione sempre rossa.

Riga 1: Modalità, tipo di precisione (`[S]` Secondi, `[F]` F-Stop) e, in Test strip, l'Avanzamento (`CO` continuo, `PP` passo-passo).

```
FOCUS
COUNTDOWN  [S]
TEST STRIP [F]PP
```

Riga 2: precisione (colonne 0–6), Canale, icona ♪ (suoni attivi, sparisce col mute) e icona ⚡ (Abilitazione relè attiva).

```
 5s      Ch:A ♪⚡     precisione in Secondi: 5 s
20s      Ch:A ♪⚡     precisione in Secondi: 20 s
1/3 +02  Ch:A ♪⚡     F-Stop 1/3, Offset +2
1/12-01  Ch:B  ⚡     F-Stop 1/12, Offset -1, mute attivo
St:3     Ch:C ♪⚡     Test strip: Step numero 3
         Ch:A ♪⚡     Focus: nessuna precisione
```

All'accensione l'LCD mostra ` - R080 Timer - ` e la versione per 1,5 s, poi l'ultimo stato salvato.

## 3. Inserire il tempo

Le cifre entrano da destra come in una calcolatrice, in decimi di secondo, fino a quattro cifre; con la quinta esce la più vecchia.

| Tasti | Tempo |
|---|---|
| `5` | 0,5 s |
| `5` `0` | 5,0 s |
| `1` `2` `5` | 12,5 s |
| `1` `2` `3` `4` `5` | 234,5 s |

- La prima cifra dopo qualsiasi altra azione (Esposizione, cambio Canale, `*`/`#`, encoder…) inizia un tempo nuovo.
- Il tempo inserito diventa anche il Tempo base, con Offset zero.
- `D` azzera il tempo. Avviare un Countdown o un Test strip con tempo zero dà il beep di errore.

## 4. Precisione: Secondi e F-Stop

La precisione è il passo con cui `*`/`#` e il Test strip spostano il tempo. Ha un tipo e un valore:

| Tipo | Valori |
|---|---|
| Secondi | 1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20 s |
| F-Stop | 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64 di stop |

**Secondi.** `#` aggiunge il valore di precisione al tempo, `*` lo toglie, entro 0,1–999,9 s.

**F-Stop.** Il tempo è calcolato dal Tempo base: tempo = Tempo base × 2^(Offset / p), dove 1/p è la precisione, arrotondato al decimo. `#` e `*` spostano l'Offset di uno (fino a ±99); l'Offset resta visibile sulla riga 2 (`+02`, `-01`, ` 00`). Il passo non avviene se il tempo uscirebbe da 0,1–999,9 s o se il Tempo base è zero.

- Premere l'encoder della precisione cambia tipo: il tempo attuale diventa il Tempo base, con Offset zero.
- Ruotare l'encoder della precisione in F-Stop ricalcola il tempo dal Tempo base con lo stesso Offset e il nuovo valore (es. Offset +2 a 1/3 diventa +2 a 1/2).

Esempio: tempo 10,0 s, F-Stop 1/3. `#` tre volte: 12,6 → 15,9 → 20,0 s (un stop). `*` una volta dal tempo base: 7,9 s.

## 5. Modalità

### Focus

Lampada accesa a tempo libero, per comporre e mettere a fuoco.

- Pulsante principale: la lampada si accende (se l'Abilitazione relè del Canale è attiva) e il 7 segmenti conta il tempo trascorso. Una seconda pressione la spegne, con il Suono di fine.
- **Spegnimento automatico:** dopo il tempo massimo scelto nel Menu di servizio (3, 5, 10, 15 o 20 minuti; di fabbrica 5) la lampada si spegne da sola, con il Suono di fine. 10 s prima suona un avviso (due toni).
- Nessun Metronomo. Il tempo impostato non viene usato, ma resta nel Canale: cifre ed encoder della precisione (rotazione e pressione) non hanno effetto.
- Il Focus non conta come Esposizione nei contatori; il suo tempo a lampada accesa conta nelle ore di lampada.

### Countdown

Una singola Esposizione lunga quanto il tempo impostato.

- Pulsante principale: lampada accesa e conto alla rovescia sul 7 segmenti, con il Metronomo a ogni secondo intero.
- A zero la lampada si spegne (dal timer hardware, con precisione al millisecondo) e suona il Suono di fine. Il 7 segmenti torna al tempo impostato: un'altra pressione ripete la stessa Esposizione.
- Pressione durante l'Esposizione: Annullamento (lampada spenta, Suono di fine). La pressione successiva riparte dal tempo pieno. Un Countdown annullato conta comunque come Esposizione.

### Test strip

Una serie di Step cumulativi per confrontare più tempi su una sola striscia di carta. Il primo Step è il tempo impostato.

| Precisione | Step (tempi cumulativi) |
|---|---|
| Secondi, valore P | T, T+P, T+2P, … con T = tempo impostato |
| F-Stop 1/p | Tempo base × 2^((Offset + k) / p) per k = 0, 1, 2, … |

Ogni Step è calcolato dal Tempo base, quindi gli arrotondamenti non si accumulano. Se due Step arrotondati coincidono (1/64 con tempi brevi), il successivo cade 0,1 s dopo.

Esempio in Secondi: T = 5,0 s, P = 5 s → Step a 5, 10, 15, 20 s… Esempio in F-Stop 1/2 con Tempo base 8,0 s: 8,0 → 11,3 → 16,0 → 22,6 s…

Durante la prova la riga 2 dell'LCD mostra lo Step in corso (`St:n`) e il 7 segmenti il tempo cumulativo. Prima di ogni Step suona il Preavviso (tre tick a −0,3, −0,2 e −0,1 s), allo Step il beep di step. Nessun Metronomo.

**Avanzamento continuo (`CO`).** La lampada resta accesa da uno Step all'altro. Si coprono le strisce a ogni beep di step. Il pulsante principale annulla la prova.

**Avanzamento passo-passo (`PP`).** A ogni Step la lampada si spegne (al millisecondo) e il timer attende: il 7 segmenti mostra il tempo cumulativo dello Step successivo. Si sposta la carta e si preme il pulsante principale: la lampada si riaccende solo per l'incremento fino allo Step successivo.

- In pausa, `D` conclude la prova con il Suono di fine. Anche `A`, `B`, `C` e una rotazione dell'encoder della Modalità la concludono, e in più passano al Canale premuto o alla Modalità successiva o precedente.
- Con la lampada accesa il pulsante principale annulla la prova.

**Fine della prova.** La prova prosegue finché non viene annullata o conclusa; a 999,9 s cumulativi si conclude da sola. Ogni nuova prova riparte dal primo Step. Una prova intera conta come una sola Esposizione, anche se annullata.

## 6. Canali e memoria

I tasti `A`, `B` e `C` richiamano tre Canali. Ogni Canale è un'impostazione di stampa completa: Modalità, tipo e valore di precisione, Avanzamento, tempo impostato, Tempo base, Offset e Abilitazione relè. Cambiare Canale ripristina tutto; cambiare Modalità non cambia il tempo.

**Abilitazione relè.** Tenendo `#` per 1 s si alterna, per il Canale attivo, se l'Esposizione accende davvero la lampada (icona ⚡) o fa solo girare il tempo, utile per provare un ciclo a lampada spenta.

**Memoria.** Le impostazioni si salvano da sole 2 s dopo l'ultima modifica e all'avvio di ogni Esposizione: Canale attivo, i tre Canali, mute, luminosità e impostazioni del Menu di servizio. All'accensione si ritrova tutto com'era.

**Valori di fabbrica.** Per ogni Canale: Countdown, Secondi 1 s, Avanzamento continuo, 10,0 s, Abilitazione relè attiva. Globali: Canale A, mute spento, LCD livello 3, 7 segmenti livello 3, LCD in esposizione livello 1, Focus massimo 5 minuti, lingua italiana, Correzione del clock 0 ppm.

Alla prima accensione con la v2.1.0 le impostazioni delle versioni precedenti (v1 e v2.0.0) non vengono lette: il timer parte dai valori di fabbrica.

## 7. Suoni e mute

| Suono | Quando | Come suona |
|---|---|---|
| Avvio | Inizio Esposizione o ripresa passo-passo | beep acuto breve (1000 Hz, 60 ms) |
| Metronomo | Ogni secondo di un Countdown | tick grave (300 Hz, 80 ms) |
| Preavviso | 0,3 s prima di ogni Step | tre tick brevi (800 Hz, 30 ms) |
| Step | A ogni Step del Test strip | beep medio (600 Hz, 150 ms) |
| Suono di fine | Fine o Annullamento | tre toni discendenti (600, 450, 300 Hz) |
| Avviso Focus | 10 s prima dello Spegnimento automatico | due toni (450 Hz, 200 ms) |
| Errore | Avvio con tempo zero; valore rifiutato nel Menu di servizio | tono grave lungo (150 Hz, 300 ms) |
| Conferma | Mute, Abilitazione relè, valore salvato nel Menu di servizio | beep molto acuto (1200 Hz, 40 ms) |

**Mute.** Tenendo `*` per 1 s si attiva o disattiva; attivandolo il timer resta in silenzio, disattivandolo suona la conferma. Con il mute attivo suona solo il Suono di fine, perché la fine di un'Esposizione non passi inosservata; restano udibili anche i suoni del Menu di servizio.

## 8. Luci in camera oscura

- L'LCD è sempre e solo rosso. 8 livelli di luminosità per l'LCD e 8 per il 7 segmenti, più fitti ai livelli bassi.
- In Focus, a lampada spenta: `*`/`#` abbassano/alzano l'LCD; tenendo premuto `0`, `*`/`#` abbassano/alzano il 7 segmenti.
- Durante ogni Esposizione (Focus acceso compreso) l'LCD scende al livello "LCD esposizione" del Menu di servizio (di fabbrica 1, il minimo) e torna al livello normale alla fine. Il 7 segmenti resta al livello scelto.

## 9. Accensione e sicurezza

- All'accensione e dopo ogni reset il relè resta spento finché non si avvia un'Esposizione. Un'Esposizione interrotta da una mancanza di corrente non riprende mai.
- Il pulsante principale agisce solo dopo 20 ms di pressione stabile: un rimbalzo non avvia né annulla un'Esposizione.
- **Watchdog:** se il firmware si blocca, la lampada si spegne entro 250 ms e la scheda si riavvia dopo 500 ms. Al riavvio l'LCD mostra `RESET: WDT` per 3 s e il contatore dei reset da watchdog aumenta.
- Sotto 2,7 V di alimentazione la scheda si resetta (brown-out) con il relè spento.
- Se l'LCD si guasta o si scollega il timer continua con il solo 7 segmenti; l'LCD viene ricollegato da solo appena risponde.

## 10. Menu di servizio

Si apre tenendo premuto `D` mentre si accende il timer (fino alla schermata iniziale). Nel menu non si avviano Esposizioni. I testi sull'LCD sono in italiano o in inglese, a scelta nella voce Lingua/Language (di fabbrica italiano).

| Comando | Azione |
|---|---|
| Encoder della Modalità, rotazione | Scorre le voci; dentro una voce, cambia schermata |
| Encoder della Modalità, pressione | Entra nella voce; dentro una voce, conferma e salva il valore |
| `D` | Torna all'elenco delle voci senza salvare |
| Cifre | Inseriscono il valore, a calcolatrice |
| `*` | Cambia il segno del valore (dopo le cifre) |

Un valore fuori dai limiti dà il beep di errore e resta da reinserire; uno accettato dà il beep di conferma e riporta all'elenco.

| Voce | Contenuto |
|---|---|
| Info | Versione del firmware e data di compilazione |
| Contatori | Quattro schermate: Accensioni (ogni avvio, compresi i reset da watchdog), Reset watchdog, Esposizioni (Countdown e test strip, anche annullati), Ore lampada (tempo a relè acceso, Focus compreso) |
| Salute | Tre schermate: Loop max (µs), ISR max (µs), Tick persi dall'accensione |
| Test rele | La pressione accende e spegne il relè; uscendo il relè si spegne |
| Test buzzer | La rotazione sceglie uno degli 8 suoni, la pressione lo suona (1 fine, 2 errore, 3 avviso Focus, 4 avvio, 5 step, 6 preavviso, 7 metronomo, 8 conferma) |
| Test 7 segmenti | Accende tutti i segmenti (`888.8`) |
| Test LCD | Riempie tutte le celle |
| Test tastierino | Mostra l'ultimo tasto premuto; `D` si prova uscendo |
| Test encoder | `M` e `P`: posizione e numero di pressioni dei due encoder |
| Test pulsante | Premi, Premuto, Rilasciato |
| Calibrazione | Tre schermate: Log calibrazione, Correzione ppm, Durata 600 s (vedi §11) |
| LCD esposizione | Livello dell'LCD durante l'Esposizione, da 1 a 8 |
| Tempo max Focus | Minuti dello Spegnimento automatico: 3, 5, 10, 15 o 20 |
| Lingua/Language | Mostra la lingua dei testi dell'LCD (`Italiano` o `English`); ogni pressione passa all'altra lingua e la salva subito; `D` torna all'elenco, già nella nuova lingua. Il nome della voce è lo stesso nelle due lingue |
| Ripristino fabb | Dentro la voce, una pressione chiede conferma e una seconda riporta tutte le impostazioni ai valori di fabbrica, Correzione del clock e lingua comprese (i contatori restano); `D` annulla |
| Esci | Torna all'uso normale |

Nei test ogni comando va al componente in prova; `D` esce.

## 11. Calibrazione del clock

Il quarzo della scheda può scostarsi di qualche decina di parti per milione (ppm): 50 ppm sono 3 ms su 60 s. La Correzione del clock compensa questo scostamento ed è salvata con le impostazioni. Va misurata una volta; dopo un Ripristino di fabbrica torna a 0.

### Metodo principale: script `tools/calibrate.py`

Serve un PC con Python 3.10 o successivo e `pyserial` (`pip install pyserial`), e il cavo USB della scheda.

1. Chiudere ogni programma che usa la porta seriale (anche `pio device monitor`).
2. Sul timer: Menu di servizio → Calibrazione → schermata "Log calibrazione", pressione dell'encoder. L'LCD mostra `Attivo, D: esci`.
3. Sul PC: `python3 tools/calibrate.py /dev/ttyACM0` (su Windows ad esempio `COM3`).
4. Dopo 30 minuti lo script stampa, ad esempio, `Correzione del clock: +23 ppm`. Il valore è assoluto: non dipende dalla correzione già salvata.
5. Sul timer: `D` chiude il log e torna all'elenco. Premere l'encoder per rientrare in Calibrazione, ruotare fino alla schermata "Correzione ppm", digitare il valore (`2` `3`), premere `*` se è negativo, premere l'encoder per salvare.

Opzioni: `--minutes N` cambia la durata della misura; `--self-test` verifica lo script senza scheda. Se il timer si riavvia all'apertura della porta (riappare la schermata iniziale), interrompere lo script con Ctrl+C, riaprire il Menu di servizio e ripartire dal punto 2.

### Senza PC: registrazione audio di un Countdown di 600,0 s

Serve un registratore audio (lo smartphone va bene) e un programma che mostri la forma d'onda (ad esempio Audacity).

1. Disattivare il mute (icona ♪ visibile): il beep di avvio non suona col mute. Si può disattivare l'Abilitazione relè per lasciare la lampada spenta.
2. Countdown, tempo `6` `0` `0` `0` (600,0 s).
3. Avviare la registrazione vicino al timer, premere il pulsante principale e attendere il Suono di fine.
4. Sulla forma d'onda misurare il tempo dall'inizio del beep di avvio all'inizio del primo tono del Suono di fine. I tick del Metronomo, uno al secondo, aiutano a orientarsi.
5. Riavviare il timer con `D` premuto: Menu di servizio → Calibrazione, ruotare fino alla schermata "Durata 600 s". Digitare la durata misurata in centesimi di secondo (`6` `0` `0` `1` `2` per 600,12 s) e premere l'encoder.

Il timer calcola la nuova correzione partendo da quella salvata, quindi la misura si può ripetere per affinarla. Un errore di lettura di 10 ms sulla forma d'onda vale circa 17 ppm: lo script è più preciso.

## 12. Log USB

Collegando la scheda al PC (115200 baud, ad esempio `pio device monitor -b 115200`) il timer scrive una riga per evento, preceduta dai millisecondi dall'accensione:

```
t=12 BOOT reset=POWER
t=8301 START mode=CD ch=A ms=100000
t=108305 END ms=100000 measured=100000
```

Eventi: `BOOT` (con la causa `POWER` o `WDT`), `ERROR settings=factory`, `START` (Modalità `FO`, `CD`, `TS`, Canale, durata chiesta), `END` e `CANCEL` (con la durata misurata dal timer), `LCD offline` / `LCD online`, `CAL` (log di calibrazione), `LOST lines=n` (righe scartate perché il PC non leggeva).

## 13. Riferimento rapido

| Voglio… | Faccio… |
|---|---|
| Esporre per 12,5 s | Countdown, `1` `2` `5`, pulsante principale |
| Ripetere l'ultima Esposizione | Pulsante principale |
| Annullare un'Esposizione | Pulsante principale |
| Un terzo di stop in più | F-Stop 1/3, `#` |
| Una striscia in passo-passo | Test strip, pressione dell'encoder della Modalità fino a `PP` |
| Silenziare il timer | `*` tenuto 1 s |
| Esporre senza accendere la lampada | `#` tenuto 1 s (sparisce ⚡) |
| Abbassare le luci | Focus a lampada spenta: `*` (LCD), `0` + `*` (7 segmenti) |
| Aprire il Menu di servizio | `D` tenuto all'accensione |
