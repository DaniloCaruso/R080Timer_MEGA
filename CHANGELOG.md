# Changelog

Versionamento semantico. Ogni rilascio è un tag `vX.Y.Z` sul commit verificato con la Checklist di collaudo firmata.

## [2.1.0] — in collaudo

Testi dell'LCD in italiano e in inglese, con la lingua scelta dal Menu di servizio.

### Da fare per aggiornare dalla v2.0.0

- Le impostazioni della v2.0.0 non vengono lette (blocco impostazioni alla versione 2): alla prima accensione il timer parte dai valori di fabbrica, in italiano.

### Aggiunto

- Voce `Lingua/Language` nel Menu di servizio, prima del ripristino di fabbrica: ogni pressione passa da italiano a inglese e viceversa, la scelta vale subito e si salva.
- Tutti i testi dell'LCD in italiano e in inglese: schermata principale (Avanzamento `CO`/`PP`, in inglese `CO`/`SS`), messaggio `RESET: WDT`, Menu di servizio completo. Nomi delle Modalità, schermata di avvio e numeri restano uguali; il log su USB resta in inglese.
- Test sul PC che bloccano traduzioni mancanti, troppo lunghe per la loro posizione o con caratteri che l'LCD non mostra.

### Modificato

- Il ripristino di fabbrica riporta anche la lingua all'italiano.
- README rinnovati con banner, badge e link al sito.

## [2.0.0] — in collaudo

Riscrittura completa del firmware per Arduino Mega 2560. Stesse funzioni e stessa mappa dei comandi della v1, con i difetti corretti e una base più sicura e misurabile. Il firmware v1 resta al tag `pre-rewrite`.

### Da fare per aggiornare dalla v1

- Spostare il filo del 7 segmenti da A2 a D14 (TX3).
- Aggiungere una resistenza da 10 kΩ tra D47 e GND.
- Le impostazioni della v1 non vengono lette: alla prima accensione il timer parte dai valori di fabbrica.
- Calibrare il clock (manuale, §11).

### Aggiunto

- Tempo di esposizione contato da Timer1 a 1 kHz: il relè si spegne dalla ISR al millisecondo, a fine Countdown e a ogni Step passo-passo.
- Correzione del clock in ppm (±1000), con log di calibrazione su USB, script `tools/calibrate.py` e calcolo dalla durata misurata di un Countdown di 600,0 s.
- Spegnimento automatico del Focus (3, 5, 10, 15 o 20 minuti, di fabbrica 5) con avviso 10 s prima.
- Menu di servizio (`D` all'accensione): info, contatori persistenti, salute del sistema, test di relè, buzzer, 7 segmenti, LCD, tastierino, encoder e pulsante, calibrazione, luminosità LCD in esposizione, tempo massimo Focus, ripristino di fabbrica.
- Watchdog con relè spento e messaggio `RESET: WDT` al riavvio.
- Log degli eventi su USB a 115200 baud.
- Luminosità a 8 livelli per LCD e 7 segmenti, salvate; LCD attenuato durante l'Esposizione.
- Preavviso di tre tick prima di ogni Step del test strip; suoni distinti per ogni evento, in un'unica tabella.
- Offset F-Stop sempre visibile sull'LCD.
- Test automatici sul PC (`pio test -e native`) e CI su GitHub.

### Modificato

- Ogni Canale memorizza l'impostazione di stampa completa: Modalità, precisione, Avanzamento, tempo, Tempo base, Offset, Abilitazione relè.
- Impostazioni in EEPROM in un blocco con versione e CRC8, salvate 2 s dopo l'ultima modifica e all'avvio di ogni Esposizione.
- Gli Step del test strip si calcolano sempre dal Tempo base: niente errori di arrotondamento accumulati.
- In Focus `*`/`#` regolano l'LCD; `0` + `*`/`#` il 7 segmenti.
- Il pulsante principale agisce alla pressione, con antirimbalzo di 20 ms.
- Il 7 segmenti usa la seriale hardware Serial3 al posto di SoftwareSerial.
- Driver LCD proprio, senza attese: un LCD guasto o scollegato non ferma il timer.
- Architettura a tre strati: dominio in C++ puro (`lib/r080_core/`), driver (`src/drivers/`), `src/main.cpp`.

### Corretto

- `*` e `#` tenuti premuti non eseguono più anche l'azione breve al rilascio.
- Le cifre digitate dopo un'Esposizione iniziano un tempo nuovo invece di accodarsi.
- Il 7 segmenti torna al tempo impostato a fine Esposizione.
- Il test strip riparte dal primo Step dopo un Annullamento.

### Rimosso

- Librerie `ArduinoSTL`, `EncoderButton`, `LiquidCrystal_I2C` e la libreria Waveshare in `lib/`; restano solo `Encoder`, `Keypad` e `OneButton`.
- `TECNICO.pdf`, `DIAGRAMS.pdf`, `PINOUT.pdf` e `DIAGRAMS.md`; la documentazione tecnica è in `TECNICO.md`.
