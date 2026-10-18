# US-003 — Fase C — Verifica hardware (operatore umano)

Rebase della state machine SoundPlayer sul clock Timer1 ISR (`g_tickMs`)
al posto di `millis()` (Timer0 Arduino core). Unica sorgente temporale in
tutto il firmware.

## Precondizioni

- [ ] Firmware compilato: `pio run` → `[SUCCESS]` exit 0, zero warning nuovi.
- [ ] Firmware flashato sul Mega 2560 del banco di test.
- [ ] Alimentazione stabile (USB o alimentatore esterno), LCD acceso, banner
      seriale `=== R080 TIMER PRONTO ===` osservato a 9600 baud.
- [ ] Mute OFF (icona ♪ visibile su LCD row 1 col 14). Se assente, hold `*`.

## Reference aurale: versione Libraries_Refactoring

Ascoltare e memorizzare la versione `Libraries_Refactoring` (pre-Fase A)
delle seguenti sequenze audio, come baseline di confronto:
- sequenza stop manuale,
- tick metronomo countdown,
- step-beep TEST_STRIP,
- warning-beep pre-step (3 beep brevi),
- beep di conferma unmute.

> L'accettazione di questa user story include "nessuna differenza percepibile
> a orecchio" rispetto a questa baseline.

## Checklist hardware

### 1. COUNTDOWN 30.0 s — metronomo (1 tick/sec)

1. [ ] Canale 1 (`A`), tastierino: `3`, `0`, `0` (→ timeSet = 300 decis = 30.0 s).
2. [ ] Encoder sinistro: selezionare `COUNTDOWN`.
3. [ ] Premere main button. Contare i tick metronomo fino all'auto-stop.

| Campione | # tick conteggiati | Attesi | Pass/Fail |
|----------|--------------------|--------|-----------|
| 1        |                    | 30     |           |
| 2        |                    | 30     |           |
| 3        |                    | 30     |           |

- [ ] Tutti e 3 i campioni = 30 tick.
- [ ] Ogni tick distanziato da ~1 s (non percepibile drift).

### 2. Stop manuale — sequenza 5 beep TONE_DOWN

1. [ ] Avviare un COUNTDOWN qualsiasi (es. 60.0 s).
2. [ ] Premere main button mid-run per stop manuale.

- [ ] Udita sequenza: `beep-pausa-beep-pausa-beep` (5 eventi sonori totali
      con pattern 75/150p/75/150p/150 ms).
- [ ] Primi due beep brevi ( ~75 ms ), terzo beep più lungo ( ~150 ms ).
- [ ] Nessun beep tagliato / sovrapposto / mancante.
- [ ] A orecchio indistinguibile dal baseline Libraries_Refactoring.

### 3. TEST_STRIP CONTINOUS SECONDS 2s — step beep a t = 10, 12, 14, 16 s

1. [ ] Encoder sinistro: `TEST_STRIP`. Mode button per impostare `CO` (CONTINOUS).
2. [ ] Encoder destro (precisione): `S` (SECONDS), valore `2`.
3. [ ] `appoTimeSet` → impostare da tastiera `1`, `0`, `0` (10.0 s come
      target iniziale).
4. [ ] Main button. Timer conta in alto partendo da 0.

| Evento            | t atteso | Step beep udito (Y/N) | Pass/Fail |
|-------------------|----------|-----------------------|-----------|
| primo step        | 10.0 s   |                       |           |
| secondo step      | 12.0 s   |                       |           |
| terzo step        | 14.0 s   |                       |           |
| quarto step       | 16.0 s   |                       |           |

- [ ] Beep TONE_UP ~150 ms su ciascuno dei 4 step.
- [ ] Interval tra beep consecutivi = 2.0 s (±40 ms ad orecchio).
- [ ] Stop manuale dopo 16 s → sequenza stop regolare.

### 4. TEST_STRIP warning beep — 3 beep brevi pre-step

Come punto 3, ma focus sul beep di preavviso.

- [ ] Prima di ogni step beep, udibili 3 beep brevi (~50 ms cad.) al ritmo
      di 1 per decisecondo, negli ultimi 3 decis prima del target.
- [ ] Beep di preavviso TONE_UP (tonalità alta) distinti dal beep di
      passaggio step.
- [ ] A orecchio indistinguibile dal baseline Libraries_Refactoring.

### 5. Toggle mute (hold `*`) — beep conferma solo sull'unmute

1. [ ] Da stato mute OFF (icona ♪ visibile), hold `*` per ~1 s.
      - [ ] LCD icona ♪ scompare.
      - [ ] NESSUN beep di conferma all'attivazione del mute.
2. [ ] Da stato mute ON, hold `*` per ~1 s.
      - [ ] LCD icona ♪ ricompare.
      - [ ] Beep TONE_UP ~200 ms udibile di conferma unmute.

### 6. Non-regressione sequenze sovrapposte

1. [ ] Avviare COUNTDOWN 5.0 s.
2. [ ] Main button per stop manuale — parte sequenza stop.
3. [ ] Ripremere main button immediatamente durante la sequenza stop ancora in corso.
- [ ] La sequenza stop in corso NON viene interrotta / resettata (regola
      `emitStopSound()` rifiuta sequenze sovrapposte — `soundPlayer.active` gate).
- [ ] Comportamento identico al baseline Libraries_Refactoring.

### 7. Non-regressione Fase A e Fase B

- [ ] COUNTDOWN 60.0 s si ferma entro ±100 ms del punto previsto (contratto Fase B).
- [ ] Relè ingranditore (pin D47) commuta OFF puntualmente a fine COUNTDOWN.
- [ ] Repeat-last-exposure: dopo stop naturale, timeSet torna al valore
      pre-partenza (non 0).

## Esito complessivo

- [ ] Tutti gli item sopra marcati Pass.
- [ ] Confronto a orecchio con Libraries_Refactoring: nessuna differenza
      percepibile in nessuna delle sequenze audio.

Firma operatore: _______________________  Data: __________
