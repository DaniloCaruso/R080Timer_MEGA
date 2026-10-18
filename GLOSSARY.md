# R080 Timer

Timer per ingranditore fotografico da camera oscura: decide per quanto tempo la lampada dell'ingranditore impressiona la carta.

## Esposizione

**Esposizione**:
Intervallo in cui la lampada dell'ingranditore è accesa per impressionare la carta.
_Avoid_: run, sessione, accensione

**Tempo impostato**:
Durata, in decimi di secondo, che il canale attivo userà per la prossima esposizione.
_Avoid_: timeSet, tempo set

**Annullamento**:
Interruzione di un'esposizione in corso tramite il pulsante principale.
_Avoid_: stop (riservato all'unità fotografica)

**Abilitazione relè**:
Impostazione del canale che decide se un'esposizione accende davvero la lampada o fa solo girare il tempo.
_Avoid_: gate, relay gate, spina

**Pulsante principale**:
Il comando che avvia e annulla le esposizioni.
_Avoid_: main button, start

## Modalità

**Modalità**:
Il tipo di lavoro che il timer esegue: Focus, Countdown o Test strip.
_Avoid_: operazione, operation, mode

**Focus**:
Modalità con lampada accesa a tempo libero, per comporre e mettere a fuoco.
_Avoid_: messa a fuoco

**Countdown**:
Modalità che esegue una singola esposizione lunga quanto il tempo impostato.
_Avoid_: conto alla rovescia, esposizione singola

**Test strip**:
Modalità che esegue una serie di step cumulativi per confrontare più tempi su un'unica striscia di carta.
_Avoid_: provino, striscia di prova

**Step**:
Ognuno dei tempi cumulativi di un test strip.
_Avoid_: passo (ambiguo con passo-passo)

**Avanzamento**:
Il modo in cui un test strip passa da uno step al successivo: continuo (lampada sempre accesa, beep a ogni step) o passo-passo (lampada spenta tra gli step, ripresa col pulsante principale).
_Avoid_: modalità, modality, CO/PP

**Spegnimento automatico**:
Spegnimento della lampada in Focus allo scadere del tempo massimo scelto nel menu di servizio.
_Avoid_: timeout

## Stop e precisione

**Stop**:
Unità logaritmica dell'esposizione: un stop in più raddoppia il tempo, uno in meno lo dimezza.
_Avoid_: diaframma, f-stop (come unità)

**Precisione**:
Il passo con cui `*`/`#` e il test strip spostano il tempo; ha un tipo (Secondi o F-Stop) e un valore.
_Avoid_: risoluzione, incremento

**Tempo base**:
Il tempo di riferimento da cui si conta l'offset; gli step F-Stop di un test strip si calcolano da tempo base e offset, a partire dal tempo impostato.
_Avoid_: baseTime, tempo di partenza

**Offset**:
Di quante frazioni di stop il tempo impostato si discosta dal tempo base.
_Avoid_: stopOffset, scostamento

## Canali

**Canale**:
Un'impostazione di stampa completa memorizzata nei tasti A, B o C: modalità, precisione, avanzamento, tempo, tempo base, offset e abilitazione relè.
_Avoid_: preset, memoria, programma

## Suoni

**Metronomo**:
Il tick che scandisce ogni secondo di un'esposizione in Countdown.
_Avoid_: beep dei secondi

**Preavviso**:
I tre tick che annunciano l'arrivo del prossimo step di un test strip.
_Avoid_: warning beep

**Suono di fine**:
La sequenza che segnala la fine o l'annullamento di un'esposizione; suona anche con il mute attivo.
_Avoid_: stop sound

**Mute**:
Stato in cui il timer non emette suoni, a parte il suono di fine.
_Avoid_: silenzioso

## Servizio

**Menu di servizio**:
Il menu che si apre tenendo premuto `D` all'accensione, con diagnostica, test hardware e impostazioni di sistema.
_Avoid_: menu nascosto, setup

**Correzione del clock**:
Il fattore, in parti per milione, che compensa la deriva dell'oscillatore della scheda.
_Avoid_: calibrazione (come nome del valore)

**Checklist di collaudo**:
La verifica manuale sulla scheda, firmata dall'operatore, che accompagna ogni rilascio.
_Avoid_: verify, test hardware
