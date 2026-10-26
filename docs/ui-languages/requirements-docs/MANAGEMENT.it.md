# MANAGEMENT — Testi dell'LCD in più lingue

- **Feature:** `ui-languages`
- **Data:** 2026-10-26
- **Fonte delle decisioni:** sessione di grilling del 2026-10-26; `docs/ui-languages/decisions.md`; `EARS.it.md` e `TECHNICAL.it.md` di questa cartella.

## Sintesi

Il timer oggi mostra sull'LCD un misto di italiano e inglese: il Menu di servizio è tutto in italiano, la schermata principale usa nomi inglesi e sigle italiane. Con questa funzione tutte le scritte dell'LCD sono disponibili in italiano e in inglese, con la lingua scelta da una nuova voce del Menu di servizio. Il lavoro è piccolo, tocca solo il firmware e la documentazione e porta il firmware alla v2.1.0. Perimetro e traduzioni sono approvati.

## Perché

- Il repository e il sito sono pubblici e bilingui: chi non parla italiano oggi non capisce il Menu di servizio, dove ci sono test e calibrazione.
- Prima: lingua fissa e mista. Dopo: italiano o inglese a scelta, con le stesse posizioni sullo schermo.
- Chi usa il timer in italiano non vede differenze: i testi italiani restano quelli di oggi.

## Cosa

**Dentro:**
- tutte le scritte dell'LCD in italiano e in inglese: schermata principale, messaggio dopo un blocco del firmware, Menu di servizio completo;
- voce `Lingua/Language` nel Menu di servizio, con lo stesso nome nelle due lingue;
- lingua salvata con le altre impostazioni; italiano come lingua di fabbrica;
- controllo automatico che ogni traduzione esista e stia nello schermo;
- versione 2.1.0, manuali e sito aggiornati, README rinnovati con banner e badge.

**Fuori:**
- altre lingue oltre a italiano e inglese: la struttura le permette, ma oggi non servono;
- lettere accentate: lo schermo non le ha e gli slot per i simboli personalizzati sono quasi tutti occupati;
- traduzione dei nomi delle Modalità (FOCUS, COUNTDOWN, TEST STRIP): sono nomi già noti, usati anche su sito e manuali;
- log su USB: resta in inglese, perché è uno strumento diagnostico;
- recupero delle impostazioni della v2.0.0: nessuna scheda la usa ancora.

## Come

1. Raccogliere tutte le scritte in un'unica tabella con le due traduzioni.
2. Aggiungere la lingua alle impostazioni salvate.
3. Far usare la tabella alla schermata principale e al Menu di servizio, con la nuova voce Lingua.
4. Aggiornare versione, changelog, manuali e README nella stessa consegna del firmware.
5. Dopo l'integrazione, aggiornare il sito con una consegna separata.

## Effort

da stimare

## Rischi

| Rischio | Probabilità | Impatto | Mitigazione |
|---|---|---|---|
| Traduzione troppo lunga, tagliata sullo schermo | Media | Basso | Controllo automatico della lunghezza a ogni modifica |
| Impostazioni azzerate passando alla v2.1.0 | Certa | Basso | Nessuna scheda ha la v2.0.0; nota nel changelog |
| Scritte illeggibili solo sulla scheda vera | Bassa | Medio | Verifica a vista nel collaudo sulla scheda (#16) |

## Criteri di successo

- Tutte le scritte dell'LCD compaiono nella lingua scelta (REQ-1.2), tranne nomi delle Modalità, schermata di avvio e numeri, uguali nelle due lingue (REQ-1.3).
- In italiano lo schermo è identico a oggi (REQ-1.7).
- La lingua si cambia dal Menu di servizio, vale subito e resta dopo lo spegnimento (REQ-2.1–2.5, REQ-3.1).
- Il ripristino di fabbrica riporta all'italiano (REQ-3.3).
- I test automatici bloccano traduzioni mancanti, troppo lunghe o con caratteri non visualizzabili (REQ-4.1–4.3).
- Firmware v2.1.0 con manuali, README e sito aggiornati (REQ-5.1–5.4).

## Decisione richiesta

Perimetro, tabella dei testi e questioni aperte approvati il 2026-10-26: il lavoro può partire.

## Questioni aperte

- **Q-1:** risolta, traduzioni approvate.
- **Q-2:** risolta, per cambiare lingua si entra nella voce e si preme.
- **Q-3:** risolta, README e manuali nel ramo del firmware, su `main` solo il sito.
