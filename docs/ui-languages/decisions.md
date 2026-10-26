# Decisioni — ui-languages

Testi dell'LCD in più lingue, con la lingua scelta dal Menu di servizio. Decisioni prese nella sessione di grilling del 2026-10-26 sul firmware v2.0.0 per Arduino Mega 2560.

## Lingue

**Decisione:** italiano e inglese. La struttura permette di aggiungere altre lingue in seguito senza ristrutturare il codice.

## Lingua predefinita

**Decisione:** italiano alla prima accensione e dopo il Ripristino di fabbrica. Il Ripristino di fabbrica riporta anche la lingua all'italiano, come tutte le altre impostazioni.

## Perimetro

**Decisione:** si traducono tutti i testi dell'LCD: schermata principale, `RESET: WDT`, tutto il Menu di servizio (titoli, voci, schermate, conferme, test). Il log su USB resta solo in inglese: è diagnostico e potrebbe essere letto da uno script.

## Caratteri

**Contesto:** la ROM dell'LCD non ha lettere accentate; degli 8 slot CGRAM due sono occupati da nota e spina.
**Decisione:** solo ASCII semplice, senza accenti (`Si`, `Modalita`) e senza nuovi glifi.

## Nomi delle Modalità

**Decisione:** `FOCUS`, `COUNTDOWN` e `TEST STRIP` restano uguali in tutte le lingue: sono nomi delle Modalità, già usati su LCD, sito, README e glossario. Si traducono solo le scritte descrittive.

## Salvataggio

**Contesto:** un byte in più in `GlobalRecord` cambia il layout del blocco impostazioni. La v2.0.0 non è mai stata installata su una scheda.
**Decisione:** byte della lingua in `GlobalRecord`, blocco impostazioni alla versione 2, nessuna migrazione: un blocco v1 porta ai valori di fabbrica. Un valore di lingua sconosciuto porta all'italiano.

## Voce nel Menu di servizio

**Decisione:** nuova voce `Lingua/Language` al primo livello, prima del Ripristino di fabbrica, con lo stesso nome in tutte le lingue così si trova anche senza capire la lingua attuale. Il clic passa da una lingua all'altra; la scelta vale subito e si salva come le altre impostazioni.

## Test

**Decisione:** un test native verifica che ogni testo esista in tutte le lingue e stia nelle colonne disponibili sull'LCD: una traduzione mancante o troppo lunga fa fallire la CI.

## Traduzioni

**Decisione:** la tabella completa dei testi IT/EN, con la lunghezza di ognuno, sta nel TECHNICAL; l'utente la rivede prima dell'implementazione. Le voci inglesi si abbreviano quanto serve per stare in 16 colonne.

## Processo e versione

**Decisione:** feature `ui-languages`: decisioni qui, EARS/TECHNICAL/MANAGEMENT in `requirements-docs/`, issue di spec e ticket in inglese con etichetta `arduino`, PR verso `feature/arduino-mega-firmware`. Il firmware diventa v2.1.0.

## Sito e manuali

**Contesto:** README e manuale del firmware v2 vivono su `feature/arduino-mega-firmware`; su `main` c'è il README del firmware originale.
**Decisione:** README, manuali e `TECNICO.md` si aggiornano nella PR del firmware; dopo il merge si aggiorna il sito (pagine sul Menu di servizio) con una PR separata su `main`.

## README più curati (2026-10-26)

**Decisione:** il README di `main`, la pagina che si vede aprendo il repository, diventa una vetrina del progetto: banner SVG viola scuro come il sito, con l'LCD rosso, badge (CI, sito, licenza, versione), link alle pagine del sito e ai due firmware. Va con una PR su `main` separata da questa feature. Il README della v2 (`README.md`, `README-IT.md` su `feature/arduino-mega-firmware`) prende lo stesso stile dentro questa feature, insieme alla voce Lingua.

## Revisione dei documenti (2026-10-26)

**Decisione:** approvate la tabella dei testi di TECHNICAL C-1 (fra cui `SS` per il passo-passo in inglese, `600.12 s` con il punto in inglese, `RESET: WDT`, `Ch:` e `St:` uguali nelle due lingue) e la voce `Lingua/Language` a cui si accede entrando: ogni clic cambia lingua, salva e resta nella voce; `D` torna all'elenco.
