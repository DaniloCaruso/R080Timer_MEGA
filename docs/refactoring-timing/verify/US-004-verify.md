# US-004 — Fase D — Verifica hardware (operatore umano)

Metronomo e step beep test-strip migrati da logica polled (`arTime % 10 == 0`
nell'`emitMetronome` e check `timerCurrentValue >= nextStepTarget` nel callback
100 ms) a trigger ISR-driven con flag `g_metronomeFlag` / `g_nextStepReached`.

Obiettivo prestazionale: tick metronomo con **σ < 1 ms** (preferibile < 0.5 ms)
su registrazione audio, step beep test-strip entro **±1 ms** dal secondo
teorico.

## Precondizioni

- [ ] Firmware compilato: `pio run` → `[SUCCESS]` exit 0, zero warning nuovi.
- [ ] Firmware flashato sul Mega 2560 del banco di test.
- [ ] Alimentazione stabile (USB o alimentatore esterno), LCD acceso, banner
      seriale `=== R080 TIMER PRONTO ===` osservato a 9600 baud.
- [ ] Mute OFF (icona ♪ visibile su LCD row 1 col 14). Se assente, hold `*`.
- [ ] Smartphone con registratore audio pronto (es. "Voice Memos" iOS,
      "Registratore vocale" Android), posizionato a ~30 cm dal buzzer, in
      ambiente silenzioso.
- [ ] Audacity installato su PC per analisi registrazione (oppure software
      equivalente in grado di misurare timestamp di picchi sonori).

## Strumenti di misura

- **σ metronomo**: registrazione audio + misura picchi in Audacity, calcolo
  deviazione standard dei Δt fra tick successivi. `σ` si calcola come:
  σ = sqrt(Σ(Δt_i − Δt̄)² / (n−1)) dove Δt̄ = media dei Δt.
- **Errore step test-strip**: cronometro al centesimo o timestamp audio in
  Audacity; errore = |t_misurato − t_teorico|.

## Checklist hardware

### 1. COUNTDOWN 30.0 s — misura σ metronomo (acceptance criterion chiave)

1. [ ] Canale 1 (tasto `A`), tastierino: `3`, `0`, `0` (→ `timeSet` = 300 decis
      = 30.0 s).
2. [ ] Encoder sinistro: selezionare `COUNTDOWN`.
3. [ ] Avvia registratore smartphone. Attendi 2 s di silenzio.
4. [ ] Premere main button. Il countdown parte, metronomo tick ogni secondo.
5. [ ] Attendere auto-stop (sequenza 5 beep a fine).
6. [ ] Stop registrazione. Trasferisci file audio al PC.
7. [ ] Apri file in Audacity. Identifica i **30 tick** metronomo (TONE_DOWN
      80 ms, picchi facilmente distinguibili come pattern ritmato). Ignora
      la sequenza finale di 5 beep.
8. [ ] Misura timestamp (in ms) di ciascuno dei 30 tick (Audacity: selezione
      al picco → cursore → pannello "Selezione" in ms).
9. [ ] Calcola Δt_i = t_{i+1} − t_i per i = 1..29 (29 intervalli).
10. [ ] Calcola media Δt̄ e deviazione standard σ (es. in LibreOffice Calc:
      `=STDEV(...)`).
11. [ ] Registra i valori:

| Misura                      | Valore misurato | Atteso    | Pass/Fail |
|-----------------------------|-----------------|-----------|-----------|
| N tick udibili              |                 | 30        |           |
| Δt̄ medio (ms)               |                 | ~1000     |           |
| σ (ms)                      |                 | < 1.0     |           |
| σ preferibile (ms)          |                 | < 0.5     |           |
| Δt max − Δt min (ms)        |                 | < 5.0     |           |

- [ ] σ < 1 ms **DOCUMENTATO** qui con valore numerico: σ = _______ ms.
- [ ] Screenshot Audacity salvato (opzionale): _______________.
- [ ] Export CSV dei timestamp salvato (opzionale):
      `docs/refactoring-timing/verify/US-004-metronome-audacity.csv`.

### 2. TEST_STRIP CONTINOUS SECONDS 2s — step beep a 10.0 / 12.0 / 14.0 / 16.0 / 18.0 s

1. [ ] Encoder sinistro: `TEST_STRIP`. Mode button per impostare `CO`
      (CONTINOUS).
2. [ ] Encoder destro (precisione): `S` (SECONDS), valore `2`.
3. [ ] Tastiera: `1`, `0`, `0` (→ `appoTimeSet` = 100 decis = 10.0 s come
      target iniziale).
4. [ ] Avviare cronometro al centesimo (smartphone). Zero al press del main
      button.
5. [ ] Main button. Timer conta in alto partendo da 0.0.

| Evento            | t teorico (s) | t misurato (s) | Errore (ms) | Pass/Fail |
|-------------------|---------------|----------------|-------------|-----------|
| primo step        | 10.000        |                |             |           |
| secondo step      | 12.000        |                |             |           |
| terzo step        | 14.000        |                |             |           |
| quarto step       | 16.000        |                |             |           |
| quinto step       | 18.000        |                |             |           |

- [ ] Ciascun errore < 1 ms (preferibile: < 100 μs).
- [ ] Beep TONE_UP ~150 ms su ciascuno step.
- [ ] 3 beep di warning breve (50 ms cad.) udibili negli ultimi 300 ms prima
      di ogni step beep (invariato da Fase C).
- [ ] Stop manuale dopo 18 s → sequenza stop regolare (5 beep).

### 3. TEST_STRIP STEP_BY_STEP SECONDS 2s — resume dal valore corrente

1. [ ] Da TEST_STRIP, Mode button per passare a `PP` (STEP_BY_STEP).
2. [ ] Lascia precisione `S` / valore `2`, `appoTimeSet` = 100 decis.
3. [ ] Main button. Timer parte da 0.0.
4. [ ] Al primo step (10.0 s):
   - [ ] Timer si ferma sullo schermo (7-seg mostra `12.0`, il prossimo target).
   - [ ] Relè ingranditore (pin D47) OFF (LED di stato spento o voltmetro → 0 V).
   - [ ] Beep step TONE_UP 150 ms udibile.
5. [ ] Attendi ~5 s. Verifica che il timer resta fermo a `12.0`.
6. [ ] Main button (secondo click). Timer **riparte dal valore corrente**
      (10.0 s), non da 0.
   - [ ] 7-seg riprende a contare da 10.0 → 10.1 → 10.2 …
   - [ ] Relè ON.
7. [ ] Al secondo step (12.0 s):
   - [ ] Timer si ferma sullo schermo (7-seg mostra `14.0`).
   - [ ] Relè OFF.
   - [ ] Beep step.
8. [ ] Main button ancora. Timer riparte da 12.0.
9. [ ] Ripeti fino al quarto step (16.0 s).
10. [ ] Stop manuale. Sequenza stop regolare.

- [ ] Resume funziona correttamente: il valore da cui riparte è quello a cui
      il timer si era fermato (≈ pari al target appena raggiunto), non zero.
- [ ] `g_nextStepTargetMs` avanza correttamente ad ogni step (visibile
      indirettamente: display mostra il prossimo target allo stop di ogni
      step).

### 4. TEST_STRIP FSTOP precisione 2 (1/2) — sequenza step 10 → 14 → 20 → 28 decis

1. [ ] Da TEST_STRIP CONTINOUS, Precision encoder button: passa da `S` a `F`.
2. [ ] Encoder destro: valore precisione `2` (cioè 1/2 f-stop).
3. [ ] Tastiera: `1`, `0` (→ `appoTimeSet` = 10 decis = 1.0 s come base).
      Nota: la base di 10 decis è scelta per avere una sequenza aritmeticamente
      verificabile (10 × 2^0.5 ≈ 14.14; 14 × 2^0.5 ≈ 19.80; 20 × 2^0.5 ≈ 28.28;
      28 × 2^0.5 ≈ 39.60 — valori arrotondati a 14, 20, 28, 40 decis).
4. [ ] Main button. Timer parte da 0.0 s (cioè 0 decis).
5. [ ] Osserva gli step a 1.0 s / 1.4 s / 2.0 s / 2.8 s / 4.0 s.

| Step # | Target teorico (decis) | Target misurato (decis) | Pass/Fail |
|--------|------------------------|-------------------------|-----------|
| 1°     | 10 (1.0 s)             |                         |           |
| 2°     | 14 (1.4 s)             |                         |           |
| 3°     | 20 (2.0 s)             |                         |           |
| 4°     | 28 (2.8 s)             |                         |           |
| 5°     | 40 (4.0 s)             |                         |           |

- [ ] Sequenza corrisponde a `round(prev * 2^(1/2))` iterativa.
- [ ] Clamp `almeno +1 decis`: se moltiplicatore producesse ≤ prev, si
      aggiunge 1 decis. Non dovrebbe scattare per questa sequenza, ma
      documenta se osservato (es. precisione 64 ad alto moltiplicatore).

### 5. Non-regressione metronomo: tick esattamente al secondo udibile

Variante a orecchio del test 1, rapida.

1. [ ] COUNTDOWN 10.0 s. Main button. Contare 10 tick.
2. [ ] I tick sono percepibilmente equidistanti (nessun drift udibile).
3. [ ] A orecchio indistinguibile dal baseline Fase C (ma potenzialmente
      **migliore** come stabilita di tempo, essendo ora ancorato a Timer1
      invece che a un modulo su 10 del tick 100 ms).

### 6. Non-regressione Fase A / Fase B / Fase C

- [ ] FOCUS 10 s: counter avanza da 0.0 a 10.0 nel tempo corretto (±~200 ms
      tollerati, FOCUS resta polled a 100 ms).
- [ ] COUNTDOWN 60 s: termina entro ±100 ms del punto previsto.
- [ ] Repeat-last-exposure: dopo stop naturale COUNTDOWN, `timeSet` torna
      al valore pre-partenza.
- [ ] Stop sequence 5 beep (TONE_DOWN 75/150p/75/150p/150 ms) invariata.
- [ ] Warning pre-step TEST_STRIP (3 beep brevi 50 ms) invariato.
- [ ] Toggle mute (hold `*`): metronomo silente quando mute ON, ritorna
      quando mute OFF. Icona ♪ LCD coerente.

### 7. Non-regressione input UI

- [ ] Keypad A/B/C switch canale (con memoria tempo).
- [ ] Keypad D reset canale corrente.
- [ ] Keypad hold-`*` toggle mute.
- [ ] Keypad hold-`#` toggle gate relè canale corrente (icona ⚡ LCD).
- [ ] Encoder sinistro switch modalità (FOCUS/COUNTDOWN/TEST_STRIP).
- [ ] Encoder destro switch precisione.
- [ ] Encoder sinistro button: TEST_STRIP toggle CO/PP.
- [ ] Encoder destro button: COUNTDOWN / TEST_STRIP toggle S/F.

## Esito complessivo

- [ ] Tutti gli item sopra marcati Pass.
- [ ] σ metronomo < 1 ms documentato con valore numerico nel test 1.
- [ ] Errore step TEST_STRIP < 1 ms documentato nel test 2.
- [ ] Nessuna regressione funzionale rispetto a Fasi A/B/C.

Firma operatore: _______________________  Data: __________
