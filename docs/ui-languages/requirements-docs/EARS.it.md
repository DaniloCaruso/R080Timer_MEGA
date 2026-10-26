# EARS — Testi dell'LCD in più lingue

- **Feature:** `ui-languages`
- **Data:** 2026-10-26
- **Fonte delle decisioni:** sessione di grilling del 2026-10-26 (domande Q1–Q12); `docs/ui-languages/decisions.md`; lettura del codice di `feature/arduino-mega-firmware` (v2.0.0): `Screen`, `ServiceMenu`, `Settings`, `Controller`, test native.

I termini con iniziale maiuscola (Modalità, Canale, Step, Avanzamento, Menu di servizio, Ripristino di fabbrica, Esposizione) seguono `GLOSSARY.md`. I requisiti estendono quelli di `docs/firmware-rewrite/requirements-docs/EARS.it.md`.

## Legenda

| Pattern | Forma |
|---|---|
| Ubiquitous | The system **shall** … |
| Event-driven | **When** `<trigger>`, the system **shall** … |
| State-driven | **While** `<state>`, the system **shall** … |
| Unwanted | **If** `<condition>`, **then** the system **shall** … |
| Optional | **Where** `<feature>`, the system **shall** … |

## REQ-1 — Lingue e testi

### REQ-1.1 (Ubiquitous)
Il sistema **shall** offrire due lingue per i testi dell'LCD: italiano e inglese.

### REQ-1.2 (State-driven)
**While** è selezionata una lingua, il sistema **shall** mostrare in quella lingua tutti i testi dell'LCD: Avanzamento del test strip, Canale e Step della schermata principale, messaggio del reset da watchdog, e nel Menu di servizio titolo, voci, titoli delle schermate, indicazioni, conferme e test.

### REQ-1.3 (Ubiquitous)
Il sistema **shall** mostrare identici in tutte le lingue i nomi delle Modalità (`FOCUS`, `COUNTDOWN`, `TEST STRIP`), la schermata di avvio, le unità di misura (`s`, `us`, `ppm`) e i numeri.

### REQ-1.4 (Ubiquitous)
Il sistema **shall** scrivere il log degli eventi su USB solo in inglese, qualunque sia la lingua selezionata.

### REQ-1.5 (Ubiquitous)
Ogni testo dell'LCD **shall** usare solo i caratteri ASCII stampabili da 0x20 a 0x7E, senza nuovi caratteri personalizzati.

### REQ-1.6 (Ubiquitous)
L'aggiunta di una lingua **shall** richiedere soltanto i testi della nuova lingua e il suo nome, senza modifiche alla logica di schermata principale e Menu di servizio.

### REQ-1.7 (State-driven)
**While** la lingua è l'italiano, il sistema **shall** mostrare sull'LCD gli stessi testi della v2.0.0.

## REQ-2 — Scelta della lingua

### REQ-2.1 (Ubiquitous)
Il Menu di servizio **shall** offrire al primo livello, subito prima del Ripristino di fabbrica, la voce `Lingua/Language`, con lo stesso nome in tutte le lingue.

### REQ-2.2 (State-driven)
**While** la voce `Lingua/Language` è aperta, il sistema **shall** mostrare il nome della lingua selezionata scritto in quella lingua (`Italiano`, `English`).

### REQ-2.3 (Event-driven)
**When** l'utente preme il pulsante dell'encoder della Modalità nella voce `Lingua/Language`, il sistema **shall** selezionare la lingua successiva, ricominciando dalla prima dopo l'ultima.

### REQ-2.4 (Event-driven)
**When** la lingua cambia, il sistema **shall** mostrare tutti i testi successivi dell'LCD nella nuova lingua senza riavviare la scheda.

### REQ-2.5 (Event-driven)
**When** la lingua cambia, il sistema **shall** emettere il beep di conferma e salvare subito le impostazioni, come per gli altri valori del Menu di servizio.

## REQ-3 — Persistenza e valori di fabbrica

### REQ-3.1 (Event-driven)
**When** la scheda si accende, il sistema **shall** ripristinare la lingua salvata insieme alle altre impostazioni.

### REQ-3.2 (Ubiquitous)
Il valore di fabbrica della lingua **shall** essere l'italiano.

### REQ-3.3 (Event-driven)
**When** l'utente conferma il Ripristino di fabbrica, il sistema **shall** riportare anche la lingua all'italiano.

### REQ-3.4 (Event-driven)
**When** il firmware si avvia su una EEPROM con il blocco impostazioni della versione 1, il sistema **shall** usare i valori di fabbrica senza tentare migrazioni.

### REQ-3.5 (Unwanted)
**If** il blocco impostazioni è valido ma la lingua salvata non è tra quelle offerte, **then** il sistema **shall** usare l'italiano e mantenere tutte le altre impostazioni salvate.

## REQ-4 — Verifica

### REQ-4.1 (Ubiquitous)
La suite `pio test -e native` **shall** fallire se un testo dell'LCD manca o è vuoto in una delle lingue.

### REQ-4.2 (Ubiquitous)
La suite `pio test -e native` **shall** fallire se un testo dell'LCD supera, in una delle lingue, il numero di colonne previsto per la sua posizione.

### REQ-4.3 (Ubiquitous)
La suite `pio test -e native` **shall** fallire se un testo dell'LCD contiene un carattere fuori da 0x20–0x7E.

## REQ-5 — Versione e documentazione

### REQ-5.1 (Ubiquitous)
Il firmware con questa funzione **shall** avere la versione `2.1.0` e una voce in `CHANGELOG.md` che segnala il ritorno ai valori di fabbrica all'aggiornamento.

### REQ-5.2 (Ubiquitous)
Il manuale utente (`DOCS-IT.md`, `DOCS.md`), i README e `TECNICO.md` **shall** descrivere la voce `Lingua/Language`, la lingua di fabbrica e il nuovo formato del blocco impostazioni.

### REQ-5.3 (Event-driven)
**When** il firmware v2.1.0 è integrato in `feature/arduino-mega-firmware`, il sito su `main` **shall** descrivere la voce `Lingua/Language` e i testi dell'LCD in entrambe le lingue.

### REQ-5.4 (Ubiquitous)
I README della v2 (`README.md`, `README-IT.md`) **shall** aprirsi con il banner del progetto, i badge di CI, sito, licenza e versione e i link alle pagine del sito, nello stile del README di `main`.

## Questioni aperte

### Q-1 — Revisione della tabella dei testi
Risolta il 2026-10-26: la tabella di TECHNICAL C-1 è approvata così com'è (REQ-1.2, REQ-1.7).

### Q-2 — Dove si preme per cambiare lingua
Risolta il 2026-10-26: si entra nella voce e ogni clic cambia lingua restando dentro (REQ-2.2, REQ-2.3).

### Q-3 — Dove si aggiorna il README
Risolta il 2026-10-26: README e manuali si aggiornano nella PR del firmware, su `main` solo il sito (REQ-5.2, REQ-5.3).
