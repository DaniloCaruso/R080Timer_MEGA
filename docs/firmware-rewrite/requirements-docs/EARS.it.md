# EARS — Riscrittura del firmware R080 Timer

- **Feature:** `firmware-rewrite`
- **Data:** 2026-10-18
- **Fonte delle decisioni:** sessione di grilling del 2026-10-18 (giri 1–6); `docs/firmware-rewrite/decisions.md`; `GLOSSARY.md`; lettura del codice al tag `pre-rewrite`.

I termini in corsivo o con iniziale maiuscola (Esposizione, Canale, Step, Avanzamento, Tempo base, Offset, Metronomo, Preavviso, Suono di fine, Menu di servizio, Correzione del clock, Spegnimento automatico, Abilitazione relè, Annullamento) seguono `GLOSSARY.md`.

## Legenda

| Pattern | Forma |
|---|---|
| Ubiquitous | The system **shall** … |
| Event-driven | **When** `<trigger>`, the system **shall** … |
| State-driven | **While** `<state>`, the system **shall** … |
| Unwanted | **If** `<condition>`, **then** the system **shall** … |
| Optional | **Where** `<feature>`, the system **shall** … |

## REQ-1 — Precisione temporale

### REQ-1.1 (Ubiquitous)
Il sistema **shall** usare il Timer1 dell'ATmega2560 in modalità CTC con interrupt a 1 kHz come unica sorgente del tempo di esposizione.

### REQ-1.2 (Event-driven)
**When** il tempo residuo di un Countdown raggiunge zero, il sistema **shall** spegnere il relè entro 1 ms dal bordo teorico, misurato rispetto al clock della scheda.

### REQ-1.3 (Event-driven)
**When** un test strip con Avanzamento passo-passo raggiunge uno Step, il sistema **shall** spegnere il relè entro 1 ms dal bordo teorico dello Step.

### REQ-1.4 (Ubiquitous)
Dopo l'applicazione della Correzione del clock, l'errore sulla durata di un'Esposizione di 60,0 s misurata con un riferimento esterno **shall** essere al massimo 2 ms.

### REQ-1.5 (Event-driven)
**When** un test strip raggiunge uno Step, il sistema **shall** avviare il beep di step entro 1 ms dal bordo teorico dello Step.

### REQ-1.6 (State-driven)
**While** un Countdown è in corso e il mute non è attivo, il sistema **shall** emettere il Metronomo a ogni secondo intero trascorso dall'inizio dell'Esposizione, con deviazione standard del periodo inferiore a 1 ms su 30 s.

### REQ-1.7 (Ubiquitous)
Il sistema **shall** applicare al conteggio del tempo la Correzione del clock salvata, espressa in parti per milione nell'intervallo ±1000 ppm.

## REQ-2 — Sicurezza dell'esposizione

### REQ-2.1 (Event-driven)
**When** la scheda si accende o si resetta per qualsiasi causa, il sistema **shall** mantenere il relè spento finché l'utente non avvia un'Esposizione.

### REQ-2.2 (Unwanted)
**If** il loop principale non viene eseguito per più di 500 ms, **then** il sistema **shall** resettare la scheda tramite watchdog lasciando il relè spento.

### REQ-2.3 (Unwanted)
**If** la tensione di alimentazione scende sotto la soglia di brown-out di fabbrica (2,7 V), **then** il sistema **shall** resettare la scheda lasciando il relè spento.

### REQ-2.4 (Ubiquitous)
Il sistema **shall** avviare o annullare un'Esposizione solo a fronte di una pressione del pulsante principale stabile per almeno 20 ms.

### REQ-2.5 (Unwanted)
**If** le impostazioni salvate in EEPROM hanno versione sconosciuta o CRC8 errato, **then** il sistema **shall** caricare i valori di fabbrica.

### REQ-2.6 (Unwanted)
**If** l'LCD o il bus I²C non rispondono, **then** il sistema **shall** proseguire Esposizioni e conteggio del tempo usando il solo 7 segmenti e registrare l'evento nel log.

### REQ-2.7 (Ubiquitous)
Ogni comunicazione con l'LCD **shall** terminare entro un timeout anche quando il display non risponde, senza bloccare il loop principale.

### REQ-2.8 (Event-driven)
**When** la scheda si riaccende dopo una mancanza di corrente avvenuta durante un'Esposizione, il sistema **shall** restare fermo con il relè spento senza riprendere l'Esposizione.

## REQ-3 — Modalità ed esposizioni

### REQ-3.1 (Ubiquitous)
Il sistema **shall** offrire le modalità Focus, Countdown e Test strip, con il comportamento descritto nei requisiti seguenti.

### REQ-3.2 (Event-driven)
**When** il pulsante principale viene premuto in Focus a timer fermo, il sistema **shall** accendere la lampada (se l'Abilitazione relè del Canale è attiva) e mostrare sul 7 segmenti il tempo trascorso.

### REQ-3.3 (Event-driven)
**When** il Focus resta acceso per il tempo massimo configurato nel Menu di servizio (3, 5, 10, 15 o 20 minuti, default 5), il sistema **shall** eseguire lo Spegnimento automatico della lampada.

### REQ-3.4 (Event-driven)
**When** mancano 10 s allo Spegnimento automatico, il sistema **shall** emettere un avviso sonoro.

### REQ-3.5 (Event-driven)
**When** il pulsante principale viene premuto in Countdown a timer fermo con tempo impostato maggiore di zero, il sistema **shall** avviare un'Esposizione lunga quanto il tempo impostato.

### REQ-3.6 (Event-driven)
**When** il pulsante principale viene premuto durante un'Esposizione in Countdown o in Focus, il sistema **shall** eseguire l'Annullamento spegnendo il relè ed emettendo il Suono di fine.

### REQ-3.7 (Event-driven)
**When** un Countdown termina o viene annullato, il sistema **shall** mostrare di nuovo sul 7 segmenti il tempo impostato, pronto per ripetere l'Esposizione.

### REQ-3.8 (Unwanted)
**If** il pulsante principale viene premuto in Countdown o Test strip con tempo impostato pari a zero, **then** il sistema **shall** emettere un beep di errore e restare fermo.

### REQ-3.9 (Ubiquitous)
In un test strip con precisione in Secondi, il sistema **shall** porre gli Step ai tempi cumulativi T, T+P, T+2P, … dove T è il Tempo base e P il valore di precisione.

### REQ-3.10 (Ubiquitous)
In un test strip con precisione F-Stop 1/p, il sistema **shall** porre lo Step k-esimo al tempo cumulativo T·2^(k/p), calcolato ogni volta dal Tempo base e arrotondato al decimo di secondo.

### REQ-3.11 (State-driven)
**While** un test strip con Avanzamento continuo è in corso, il sistema **shall** tenere la lampada accesa ed emettere un beep di step a ogni Step.

### REQ-3.12 (Event-driven)
**When** un test strip con Avanzamento passo-passo raggiunge uno Step, il sistema **shall** spegnere la lampada e attendere una pressione del pulsante principale, dopo la quale esegue solo l'incremento fino allo Step successivo.

### REQ-3.13 (Event-driven)
**When** un test strip viene avviato dopo un Annullamento o dopo la sua conclusione, il sistema **shall** ripartire dal primo Step.

### REQ-3.14 (Event-driven)
**When** il tempo cumulativo di un test strip raggiunge 999,9 s, il sistema **shall** concludere il test strip spegnendo il relè ed emettendo il Suono di fine.

### REQ-3.15 (Event-driven)
**When** il pulsante principale viene premuto mentre la lampada è accesa in un test strip, il sistema **shall** eseguire l'Annullamento del test strip.

### REQ-3.16 (Event-driven)
**When** durante la pausa di un test strip passo-passo si preme `D`, `A`, `B` o `C` oppure si cambia modalità, il sistema **shall** concludere il test strip mostrando sul 7 segmenti il Tempo base, così che la prova successiva riparta dal primo Step.

## REQ-4 — Precisione e F-Stop

### REQ-4.1 (Ubiquitous)
Il sistema **shall** offrire i valori di precisione F-Stop 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/24, 1/32, 1/64 e i valori in Secondi 1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20.

### REQ-4.2 (Event-driven)
**When** si preme `*` o `#` in Countdown o Test strip con precisione in Secondi, il sistema **shall** diminuire o aumentare il tempo impostato del valore di precisione, entro i limiti 0,1–999,9 s.

### REQ-4.3 (Event-driven)
**When** si preme `*` o `#` in Countdown o Test strip con precisione F-Stop 1/p, il sistema **shall** diminuire o aumentare l'Offset di uno e impostare il tempo a Tempo base·2^(Offset/p), arrotondato al decimo e limitato a 0,1–999,9 s.

### REQ-4.4 (State-driven)
**While** la precisione è di tipo F-Stop, il sistema **shall** mostrare stabilmente sulla riga 2 dell'LCD il valore di precisione seguito dall'Offset nel formato segno e due cifre (`+02`, `-01`, ` 00`).

### REQ-4.5 (Event-driven)
**When** si ruota l'encoder della precisione con precisione F-Stop, il sistema **shall** ricalcolare il tempo impostato da Tempo base e Offset con il nuovo valore di precisione.

### REQ-4.6 (Event-driven)
**When** si preme il pulsante dell'encoder della precisione in Countdown o Test strip, il sistema **shall** alternare il tipo di precisione tra Secondi e F-Stop, e passando a F-Stop porre il Tempo base al tempo impostato con Offset zero.

### REQ-4.7 (Event-driven)
**When** si preme il pulsante dell'encoder della modalità in Test strip, il sistema **shall** alternare l'Avanzamento tra continuo e passo-passo.

## REQ-5 — Comandi

### REQ-5.1 (Event-driven)
**When** si digita una cifra in Countdown o Test strip, il sistema **shall** farla entrare da destra nel tempo impostato a quattro cifre, facendo uscire da sinistra la cifra più vecchia.

### REQ-5.2 (Event-driven)
**When** si digita la prima cifra dopo qualsiasi altra azione (Esposizione, cambio Canale, `*`/`#`), il sistema **shall** iniziare un tempo nuovo invece di accodare la cifra al tempo precedente.

### REQ-5.3 (Event-driven)
**When** un tempo viene inserito da tastierino, il sistema **shall** porre il Tempo base al tempo inserito con Offset zero.

### REQ-5.4 (Event-driven)
**When** si preme `D` a timer fermo, il sistema **shall** azzerare tempo impostato, Tempo base e Offset del Canale attivo.

### REQ-5.5 (Event-driven)
**When** `*` resta premuto per 1 s, il sistema **shall** alternare il mute, confermarlo con un breve beep e non eseguire l'azione di `*` al rilascio.

### REQ-5.6 (Event-driven)
**When** `#` resta premuto per 1 s, il sistema **shall** alternare l'Abilitazione relè del Canale attivo, confermarla con un breve beep e non eseguire l'azione di `#` al rilascio.

### REQ-5.7 (Event-driven)
**When** si preme `*` o `#` in Focus, il sistema **shall** diminuire o aumentare la luminosità dell'LCD.

### REQ-5.8 (Event-driven)
**When** si preme `*` o `#` in Focus tenendo premuto `0`, il sistema **shall** diminuire o aumentare la luminosità del 7 segmenti.

### REQ-5.9 (State-driven)
**While** un'Esposizione è in corso, il sistema **shall** ignorare encoder, tastierino e cambio Canale, accettando solo il pulsante principale.

### REQ-5.10 (Event-driven)
**When** l'encoder della modalità ruota di una tacca, il sistema **shall** passare alla modalità successiva o precedente rispetto a quella corrente.

### REQ-5.11 (Event-driven)
**When** il pulsante principale viene premuto, il sistema **shall** reagire appena la pressione supera l'antirimbalzo di 20 ms, senza attendere il rilascio.

### REQ-5.12 (Ubiquitous)
Il sistema **shall** registrare ogni pressione di tasto, encoder o pulsante che superi l'antirimbalzo, anche mentre aggiorna i display.

## REQ-6 — Canali e persistenza

### REQ-6.1 (Ubiquitous)
Ogni Canale A, B e C **shall** memorizzare modalità, tipo e valore di precisione, Avanzamento, tempo impostato, Tempo base, Offset e Abilitazione relè.

### REQ-6.2 (Event-driven)
**When** si preme `A`, `B` o `C` a timer fermo, il sistema **shall** attivare il Canale corrispondente ripristinandone tutta l'impostazione.

### REQ-6.3 (Event-driven)
**When** cambia la modalità, il sistema **shall** conservare il tempo impostato del Canale attivo.

### REQ-6.4 (Event-driven)
**When** la scheda si accende, il sistema **shall** ripristinare Canale attivo, impostazioni dei tre Canali, mute, luminosità e impostazioni del Menu di servizio salvati.

### REQ-6.5 (Ubiquitous)
Il sistema **shall** salvare le impostazioni in EEPROM in un formato con numero di versione e CRC8.

### REQ-6.6 (Event-driven)
**When** il firmware si avvia per la prima volta su una EEPROM con il formato precedente, il sistema **shall** usare i valori di fabbrica senza tentare migrazioni.

### REQ-6.7 (Event-driven)
**When** sono trascorsi 2 s dall'ultima cifra digitata oppure parte un'Esposizione, il sistema **shall** salvare in EEPROM le impostazioni modificate.

### REQ-6.8 (Ubiquitous)
Il sistema **shall** scrivere in EEPROM solo i byte il cui valore è cambiato.

## REQ-7 — Display e luci

### REQ-7.1 (Ubiquitous)
Il sistema **shall** illuminare l'LCD sempre e solo in rosso.

### REQ-7.2 (Ubiquitous)
Il sistema **shall** gestire luminosità separate e persistenti per LCD e 7 segmenti.

### REQ-7.3 (State-driven)
**While** un'Esposizione è in corso, il sistema **shall** portare l'LCD alla luminosità di esposizione configurata nel Menu di servizio (default: minimo visibile) e ripristinarla alla fine.

### REQ-7.4 (State-driven)
**While** un'Esposizione è in corso, il sistema **shall** mantenere il 7 segmenti alla luminosità scelta dall'utente.

### REQ-7.5 (Ubiquitous)
Il sistema **shall** mantenere la disposizione dell'LCD della versione precedente: nome della modalità, tipo di precisione e Avanzamento sulla riga 1; precisione, Canale, icona del mute e icona del relè sulla riga 2.

### REQ-7.6 (State-driven)
**While** un test strip è in corso, il sistema **shall** mostrare sulla riga 2 dell'LCD il numero dello Step corrente (`St:n`) al posto della precisione.

### REQ-7.7 (State-driven)
**While** un test strip è in corso, il sistema **shall** mostrare sul 7 segmenti il tempo cumulativo.

### REQ-7.8 (State-driven)
**While** un test strip passo-passo è in pausa tra due Step, il sistema **shall** mostrare sul 7 segmenti il tempo cumulativo dello Step successivo.

### REQ-7.9 (Event-driven)
**When** la scheda si accende, il sistema **shall** mostrare `R080 Timer` e la versione del firmware per 1,5 s e poi l'ultimo stato salvato.

### REQ-7.10 (State-driven)
**While** la modalità è Focus, il sistema **shall** non mostrare alcuna precisione sulla riga 2 dell'LCD.

### REQ-7.11 (Ubiquitous)
Il sistema **shall** offrire 8 livelli di luminosità per l'LCD e 8 per il 7 segmenti, distribuiti in modo non lineare con passi più fitti ai livelli bassi.

### REQ-7.12 (Ubiquitous)
I valori di fabbrica delle luminosità **shall** essere: 7 segmenti al livello equivalente al valore 10 su 255 della versione precedente, LCD al livello 3, LCD durante l'Esposizione al livello 1.

## REQ-8 — Suoni

### REQ-8.1 (Event-driven)
**When** inizia un'Esposizione, il sistema **shall** emettere un beep acuto breve.

### REQ-8.2 (State-driven)
**While** la modalità è Focus o Test strip, il sistema **shall** non emettere il Metronomo.

### REQ-8.3 (Event-driven)
**When** mancano 0,3 s allo Step successivo di un test strip, il sistema **shall** emettere il Preavviso di tre tick brevi.

### REQ-8.4 (Event-driven)
**When** un'Esposizione termina o viene annullata, il sistema **shall** emettere il Suono di fine di tre toni discendenti.

### REQ-8.5 (State-driven)
**While** il mute è attivo, il sistema **shall** non emettere suoni, a eccezione del Suono di fine.

### REQ-8.6 (Ubiquitous)
Avvio, Metronomo, beep di step, Preavviso, Suono di fine, beep di errore e beep di conferma **shall** essere distinguibili a orecchio l'uno dall'altro.

### REQ-8.7 (Ubiquitous)
Il sistema **shall** emettere i suoni con i parametri di partenza della tabella seguente, raccolti in un'unica tabella nel codice e ritoccabili durante il collaudo.

| Suono | Parametri |
|---|---|
| Avvio | 1000 Hz, 60 ms |
| Metronomo | 300 Hz, 80 ms |
| Step | 600 Hz, 150 ms |
| Preavviso | 3 tick a 800 Hz da 30 ms |
| Suono di fine | 600, 450, 300 Hz da 150 ms con pause di 80 ms |
| Errore | 150 Hz, 300 ms |
| Conferma | 1200 Hz, 40 ms |
| Avviso di Spegnimento automatico | 2 toni a 450 Hz da 200 ms |

## REQ-9 — Menu di servizio e diagnostica

### REQ-9.1 (Event-driven)
**When** `D` è tenuto premuto all'accensione, il sistema **shall** aprire il Menu di servizio.

### REQ-9.2 (Ubiquitous)
Il Menu di servizio **shall** mostrare versione del firmware e data di compilazione.

### REQ-9.3 (Ubiquitous)
Il Menu di servizio **shall** mostrare i contatori persistenti delle accensioni, dei reset da watchdog, del numero di Esposizioni e delle ore totali di lampada accesa.

### REQ-9.4 (Ubiquitous)
Il Menu di servizio **shall** mostrare la durata massima del loop principale, la durata massima della ISR di Timer1 e il numero di tick di Timer1 persi dall'accensione.

### REQ-9.5 (Ubiquitous)
Il Menu di servizio **shall** offrire un test per ciascun componente: relè, buzzer, 7 segmenti, LCD, tastierino, encoder e pulsante principale.

### REQ-9.6 (Event-driven)
**When** l'utente avvia il log di calibrazione dal Menu di servizio, il sistema **shall** scrivere sulla porta USB, una volta al secondo e fino all'uscita con `D`, il proprio conteggio dei millisecondi.

### REQ-9.7 (Ubiquitous)
Il Menu di servizio **shall** permettere di impostare la luminosità dell'LCD durante l'Esposizione e il tempo massimo del Focus.

### REQ-9.8 (Event-driven)
**When** l'utente conferma il ripristino di fabbrica dal Menu di servizio, il sistema **shall** sostituire tutte le impostazioni con i valori di fabbrica.

### REQ-9.9 (Event-driven)
**When** la scheda si avvia dopo un reset da watchdog, il sistema **shall** mostrare `RESET: WDT` sull'LCD per 3 s e incrementare il contatore dei reset da watchdog.

### REQ-9.10 (Ubiquitous)
Il sistema **shall** scrivere sulla porta USB a 115200 baud un log degli eventi: avvio, inizio e fine di ogni Esposizione con la durata misurata, errori.

### REQ-9.11 (Ubiquitous)
Il sistema **shall** contare come Esposizione ogni Countdown e ogni test strip intero (un test strip vale una Esposizione) e sommare nelle ore di lampada tutto il tempo a relè acceso, Focus compreso.

### REQ-9.12 (Ubiquitous)
Il Menu di servizio **shall** permettere di inserire la Correzione del clock direttamente in ppm, oppure la durata misurata di un Countdown di 600,0 s da cui calcolarla.

### REQ-9.13 (Ubiquitous)
Il repository **shall** contenere lo script `tools/calibrate.py`, che confronta per 30 minuti il log di calibrazione con l'orologio del PC e stampa la Correzione del clock in ppm.

### REQ-9.14 (Ubiquitous)
Il Menu di servizio **shall** usare l'encoder della modalità per scorrere le voci, il suo pulsante per entrare o confermare, `D` per tornare indietro, i tasti numerici per i valori, e un'ultima voce "Esci" che riavvia il timer in uso normale.

## REQ-10 — Piattaforma e hardware

### REQ-10.1 (Ubiquitous)
Il sistema **shall** girare su un Arduino Mega 2560 originale con i componenti attuali: LCD Waveshare RGB, 7 segmenti seriale, tastierino 4x4, due encoder con pulsante, pulsante principale, buzzer e relè.

### REQ-10.2 (Ubiquitous)
Il sistema **shall** pilotare il 7 segmenti tramite la seriale hardware Serial3 (pin 14).

### REQ-10.3 (Ubiquitous)
Il sistema **shall** usare come librerie esterne solo `Encoder`, `Keypad` e `OneButton`, senza `EncoderButton`, `LiquidCrystal_I2C`, `ArduinoSTL` e `arduino-timer`.

### REQ-10.4 (Ubiquitous)
Il sistema **shall** mantenere invariati tutti i collegamenti tranne quello del 7 segmenti e l'aggiunta di REQ-10.5.

### REQ-10.5 (Ubiquitous)
Il cablaggio **shall** includere una resistenza da 10 kΩ tra D47 e GND, che tiene spento il relè finché il firmware non comanda il pin.

## REQ-11 — Architettura e qualità

### REQ-11.1 (Ubiquitous)
La logica di dominio **shall** essere scritta in C++ senza dipendenze da Arduino e comunicare con l'hardware solo tramite interfacce astratte.

### REQ-11.2 (Ubiquitous)
Ogni componente hardware **shall** essere pilotato da un driver che implementa la relativa interfaccia di dominio, con `main.cpp` limitato a collegare driver e dominio.

### REQ-11.3 (Ubiquitous)
La logica di dominio, compresa la logica eseguita dalla ISR, **shall** essere coperta da test automatici Unity eseguibili sul PC con `pio test -e native`.

### REQ-11.4 (Event-driven)
**When** viene fatto un push su GitHub, la CI **shall** eseguire `pio run` per il Mega e `pio test -e native` e fallire se uno dei due fallisce.

### REQ-11.5 (Ubiquitous)
Il sistema **shall** rappresentare internamente il tempo in millisecondi su 32 bit e usare il decimo di secondo come unità di interfaccia, con tempo impostabile da 0,1 a 999,9 s.

## REQ-12 — Rilasci, repository e documentazione

### REQ-12.1 (Ubiquitous)
Il firmware **shall** seguire il versionamento semantico a partire da `v2.0.0` e registrare ogni versione in `CHANGELOG.md`.

### REQ-12.2 (Event-driven)
**When** viene pubblicato un rilascio, il progetto **shall** creare un tag git legato a una Checklist di collaudo firmata.

### REQ-12.3 (Ubiquitous)
Al termine della riscrittura il repository **shall** contenere manuale utente, `README` e `PINOUT.md` riscritti, nuovi documenti tecnici al posto di `TECNICO.md` e `DIAGRAMS.md`, nessun PDF generato, e `docs/refactoring-timing/` come archivio.

### REQ-12.4 (Ubiquitous)
Il repository **shall** usare i branch `feature/arduino-mega-firmware` e `feature/esp32-micropython-firmware` e conservare il tag `pre-rewrite` sul firmware precedente.

### REQ-12.5 (Ubiquitous)
Il manuale utente **shall** descrivere, come alternativa a `tools/calibrate.py` senza PC, la calibrazione tramite registrazione audio dei beep di inizio e fine di un Countdown di 600,0 s.

## Questioni aperte

Tutte le questioni emerse sono state risolte nel giro 7 della sessione di grilling (2026-10-18).

### Q-1 — Causa del reset e soglia di brown-out
Risolta: nella v1 soglia di fabbrica e riconoscimento del solo reset da watchdog (REQ-2.3, REQ-9.3, REQ-9.9). Soglia a 4,3 V via ISP nel backlog.

### Q-2 — Parametri dei suoni
Risolta in REQ-8.7.

### Q-3 — Scale di luminosità
Risolta in REQ-7.11 e REQ-7.12.

### Q-4 — Fine di un test strip passo-passo in pausa
Risolta in REQ-3.16.

### Q-5 — Cosa conta come Esposizione nei contatori
Risolta in REQ-9.11.

### Q-6 — Relè durante il bootloader
Risolta in REQ-10.5 (resistenza preventiva; l'utente ritiene che il modulo relè ne abbia già una).

### Q-7 — Precisione della calibrazione con cronometro manuale
Risolta in REQ-9.6, REQ-9.12, REQ-9.13 e REQ-12.5.
