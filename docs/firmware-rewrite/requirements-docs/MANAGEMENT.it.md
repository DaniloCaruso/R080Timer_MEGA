# MANAGEMENT — Riscrittura del firmware R080 Timer

- **Feature:** `firmware-rewrite`
- **Data:** 2026-10-18
- **Fonte delle decisioni:** sessione di grilling del 2026-10-18; `docs/firmware-rewrite/decisions.md`; requisiti in `EARS.it.md`; dettaglio tecnico in `TECHNICAL.it.md`.

## Sintesi

Il firmware del timer per ingranditore va riscritto da capo, mantenendo le stesse funzioni per chi lo usa in camera oscura. La versione attuale compila ma non è mai stata provata sulla scheda, e l'analisi del codice ha trovato difetti che la rendono inutilizzabile: un beep che suona all'infinito a timer fermo, il display che mostra sempre zero, la memoria dei canali che si perde, pulsanti che non rispondono. La riscrittura punta a un prodotto affidabile: esposizioni precise al millesimo di secondo, lampada che non resta mai accesa per errore, comandi sempre reattivi, logica verificata da test automatici. Tutte le questioni aperte sono state decise: si chiede di approvare lo scopo e autorizzare l'avvio dell'implementazione.

## Perché

- **Oggi il timer non è usabile.** Da fermo il buzzer ripete il suono di fine senza sosta e il display dei secondi mostra sempre zero, quindi non si vede il tempo che si digita.
- **Le impostazioni si perdono.** Il tempo salvato per ogni canale viene azzerato all'accensione e a ogni cambio di modalità.
- **I comandi sono lenti.** Il programma impiega circa un decimo di secondo per ogni giro, a causa di attese inutili nella gestione del display: le pressioni brevi del pulsante di avvio vanno perse.
- **Nessuna verifica.** I documenti del lavoro precedente dichiarano obiettivi raggiunti "per costruzione", ma nessuna prova sulla scheda è stata fatta e alcune affermazioni non corrispondono al codice.
- **Nessuna rete di sicurezza.** Se il programma si blocca durante un'esposizione, nulla spegne la lampada.

Dopo la riscrittura: errore massimo di 2 millesimi di secondo su un'esposizione di 60 secondi, lampada spenta automaticamente in caso di blocco entro mezzo secondo, comandi che rispondono alla pressione.

## Cosa

**Nel perimetro**
- Le funzioni di oggi: messa a fuoco, esposizione singola, prova a strisce (continua e passo-passo), regolazione in secondi o in frazioni di stop, tre memorie di stampa, silenziamento, esclusione della lampada per canale.
- Ogni memoria salva l'intera impostazione di stampa, e all'accensione si ritrova tutto com'era.
- Sicurezza: lampada sempre spenta all'accensione e in caso di guasto, blocco automatico se il programma si ferma, impostazioni protette contro la corruzione dei dati, spegnimento automatico della messa a fuoco dopo un tempo configurabile.
- Uso al buio: display sempre rosso, luminosità regolabili separatamente, display attenuato durante l'esposizione, suoni distinguibili.
- Menu di servizio: calibrazione dell'orologio (con uno script sul computer o, senza computer, registrando l'audio dei beep), diagnostica, test di ogni componente, ritorno ai valori di fabbrica, registro eventi leggibile dal computer.
- Test automatici sul computer e controllo automatico a ogni modifica del codice.
- Due piccole modifiche al cablaggio: il cavo del display dei secondi spostato su un altro pin, per eliminare l'ultima fonte di ritardo, e una resistenza sul pin del relè che garantisce la lampada spenta all'accensione.

**Fuori dal perimetro (nel backlog)**
- Protezione più alta contro i cali di tensione: richiede un programmatore esterno da usare una sola volta.
- Pausa e ripresa dell'esposizione, compensazione dell'asciugatura, programmi salvati, pedale, comando della luce di sicurezza: sono funzioni nuove e si valutano solo dopo che la base è stabile.
- Ripresa di un'esposizione interrotta da mancanza di corrente: esclusa per sempre, perché alla riaccensione la carta potrebbe essersi spostata.
- La versione per ESP32 segue un percorso separato.

## Come

1. **Preparazione:** rinomina dei branch, salvataggio della versione attuale con un'etichetta, ambiente per i test sul computer e controllo automatico su GitHub.
2. **Logica di dominio:** il comportamento del timer viene scritto senza dipendere dalla scheda e verificato da test automatici prima di toccare l'hardware.
3. **Collegamento all'hardware:** componenti sottili per relè, display, tastierino, encoder, buzzer, memoria e sicurezza.
4. **Sostituzione:** il nuovo firmware prende il posto del vecchio in un solo passaggio.
5. **Collaudo e rilascio:** prove sulla scheda secondo una checklist firmata, documentazione aggiornata, rilascio della versione 2.0.0.

## Effort

*da stimare*: l'intervista non ha fissato una stima.

## Rischi

| Rischio | Probabilità | Impatto | Mitigazione |
|---|---|---|---|
| Con la protezione di fabbrica, un calo di tensione moderato può far comportare la scheda in modo imprevedibile e non viene riconosciuto | Bassa | Medio | Accettato per la prima versione; protezione più alta con un programmatore esterno nel backlog |
| La lampada si accende per un istante all'accensione | Bassa | Alto | Resistenza preventiva sul pin del relè |
| Calibrazione dell'orologio imprecisa | Bassa | Medio | Misura automatica di 30 minuti con il computer al posto del cronometro a mano |
| Una riscrittura completa introduce difetti nuovi | Media | Alto | Test automatici su tutta la logica, checklist di collaudo firmata per ogni rilascio, versione precedente sempre recuperabile |
| Memoria della scheda insufficiente | Bassa | Medio | Oggi è usato il 18% della RAM; controllo a ogni compilazione |
| Usura della memoria dei contatori dopo circa 100 000 esposizioni | Bassa | Basso | Accettabile per la prima versione |

## Criteri di successo

- Esposizione di 60 secondi con errore massimo di 2 millesimi di secondo dopo la calibrazione (REQ-1.4).
- Lampada spenta entro 1 millesimo di secondo dalla fine dell'esposizione, anche nella prova a strisce passo-passo (REQ-1.2, REQ-1.3).
- Metronomo regolare, con variazione inferiore a 1 millesimo di secondo (REQ-1.6).
- Lampada sempre spenta all'accensione e dopo qualsiasi reset, nessuna esposizione ripresa dopo un'interruzione (REQ-2.1, REQ-2.8).
- Reset automatico entro mezzo secondo se il programma si blocca (REQ-2.2).
- Un display guasto non interrompe l'esposizione (REQ-2.6).
- Nessuna pressione di tasto persa; avvio immediato alla pressione del pulsante (REQ-5.11, REQ-5.12).
- Ogni canale ritrova la sua impostazione completa, anche dopo lo spegnimento (REQ-6.1, REQ-6.4).
- Display sempre rosso e attenuato durante l'esposizione (REQ-7.1, REQ-7.3).
- Tutta la logica verificata da test automatici a ogni modifica (REQ-11.3, REQ-11.4).
- Rilascio 2.0.0 con checklist di collaudo firmata (REQ-12.1, REQ-12.2).

## Decisione richiesta

Approvare lo scopo descritto e autorizzare l'avvio dell'implementazione nell'ordine indicato. In alternativa, indicare le parti dello scopo da modificare.

## Questioni aperte

Nessuna. Le sette questioni emerse sono state decise nell'ultimo giro di domande:

- **Q-1 — Causa del reset per calo di tensione:** per ora si tiene la protezione di fabbrica e si riconoscono solo i blocchi del programma; la protezione più alta va nel backlog.
- **Q-2 — Suoni:** fissati tono e durata di ogni suono, da ritoccare a orecchio durante il collaudo.
- **Q-3 — Luminosità:** 8 livelli per display, più fitti nelle luminosità basse; il display dei secondi parte dalla luminosità di oggi.
- **Q-4 — Fine della prova a strisce passo-passo:** il tasto D chiude la prova ferma tra due step, come anche il cambio di canale o di modalità.
- **Q-5 — Conteggio delle esposizioni:** contano l'esposizione singola e la prova a strisce (una volta per prova); la messa a fuoco conta solo nelle ore di lampada.
- **Q-6 — Lampada all'accensione:** si monta una resistenza preventiva sul pin del relè.
- **Q-7 — Metodo di calibrazione:** script sul computer come metodo principale, registrazione audio come alternativa.
