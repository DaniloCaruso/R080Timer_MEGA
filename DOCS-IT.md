# R080 TIMER MEGA - Timer per Ingranditore Fotografico

## Panoramica Generale

Questo progetto implementa un timer digitale avanzato per ingranditore fotografico, basato su Arduino Mega. Il dispositivo offre diverse modalità operative, tra cui Focus, Countdown e Test Strip, con controlli precisi per la gestione dei tempi di esposizione.

### Componenti Principali

0. Arduino Mega 2560
1. Display LCD 16x2
2. Display a 7 segmenti
3. Tastierino numerico
4. Encoder rotativo per la selezione della modalità
5. Encoder rotativo per la regolazione della precisione
6. Pulsante principale per avvio/arresto
7. Relè per il controllo dell'ingranditore

### Modalità Operative

1. **Focus**: Conteggio del tempo senza limite.
2. **Countdown**: Conto alla rovescia da un tempo prestabilito.
3. **Test Strip**: Esposizioni incrementali per prove di stampa.

## Architettura del Software

Il progetto è organizzato in diversi file header che gestiscono funzionalità specifiche:

- `types.h`: Definisce strutture dati e enumerazioni.
- `constants.h`: Contiene costanti globali.
- `variables.h`: Dichiara variabili globali.
- `config.h`: Gestisce la configurazione iniziale dei componenti.
- `presentation.h`: Funzioni per la visualizzazione su LCD e 7 segmenti.
- `keypad_mgmt.h`: Gestione del tastierino numerico.
- `encoder_mgmt.h`: Gestione degli encoder rotativi.
- `running_mgmt.h`: Gestione dello stato di esecuzione e del pulsante principale.
- `operations.h`: Definisce e gestisce le operazioni disponibili.
- `actions.h`: Implementa le azioni specifiche per ciascuna modalità.

## Flusso di Esecuzione

1. **Inizializzazione**:
   - Configurazione dei pin e dei componenti hardware.
   - Inizializzazione delle operazioni disponibili.

2. **Loop Principale**:
   - Gestione del pulsante principale (`manageMainButtonAction()`).
   - Valutazione degli input da encoder e tastierino.
   - Esecuzione dell'azione corrente in base alla modalità selezionata.

3. **Gestione degli Eventi**:
   - Gli interrupt gestiscono gli eventi del pulsante principale.
   - Le funzioni di callback gestiscono gli eventi degli encoder.

4. **Cambio di Modalità**:
   - L'encoder di modalità modifica `operationStatus.current`.
   - La presentazione su LCD viene aggiornata di conseguenza.

5. **Esecuzione delle Azioni**:
   - Ogni modalità (Focus, Countdown, Test Strip) ha la propria funzione di esecuzione.
   - Le azioni sono avviate/interrotte dal pulsante principale.

## Dettagli Implementativi

### Gestione del Pulsante Principale

Il pulsante principale utilizza un sistema di interrupt per la massima reattività:
```cpp
void mainButtonHandler() {
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime > DEBOUNCE_TIME) {
        mainBtnEventOccurred = true;
        lastInterruptTime = interruptTime;
    }
}
```


La funzione `manageMainButtonAction()` in `running_mgmt.h` gestisce l'azione da intraprendere in base allo stato corrente.

### Struttura delle Operazioni

Le operazioni sono definite come oggetti della classe `Operation`:

```cpp
class Operation {
    public:
        String name;
        byte id;
        ModalityTypeStatus modalityTypeStatus;
        PrecisionTypeStatus precisionTypeStatus;
        PrecisionStatus precisionStatus;
        ChannelInfo channelsInfo[CHANNELS_IDS::SIZE];
        void (showPresentation)();
        void (switchModality)(ModalityTypeStatus );
        void (switchPrecisionType)(PrecisionTypeStatus );
        void (startedAction)();
// ...
}
```

### Gestione degli Encoder e dei Pulsanti

Gli encoder e i pulsanti sono gestiti tramite le librerie `Encoder` e `EncoderButton`:

```cpp
// Definizione degli encoder
Encoder encoderMode(ENC_MODE_DT_PIN, ENC_MODE_CLK_PIN);
Encoder encoderPrecision(ENC_PRECISION_DT_PIN, ENC_PRECISION_CLK_PIN);

// Definizione dei pulsanti degli encoder
EncoderButton encoderModeButton = EncoderButton(ENC_MODE_SW_PIN);
EncoderButton encoderPrecisionButton = EncoderButton(ENC_PRECISION_SW_PIN);

// Gestione dell'encoder di modalità
void evaluateEncoderMod() {
  long newEncoderPosition = encoderMode.read() >> 2;
  int timerModeIndex = (newEncoderPosition % 3 + 3) % 3;
  setOperationStatus(timerModeIndex);
  showModalityType();
}

// Gestione dell'encoder di precisione
void evaluateEncoderPrec() {
  long newEncoderPosition = encoderPrecision.read() >> 2;
  int precisionIndex = (newEncoderPosition % 11 + 11) % 11;
  setOperationPrecision(precisionIndex);
  showPrecision();
}

// Gestione del pulsante di modalità
void onModeButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchModality(&operationStatus.current.modalityTypeStatus);
}

// Gestione del pulsante di precisione
void onPrecisionButtonEvent(EncoderButton& encoderButton) {
  operationStatus.current.switchPrecisionType(&operationStatus.current.precisionTypeStatus);
}

// Registrazione dei gestori di eventi per i pulsanti
void registerClickHandler() {
  encoderModeButton.setClickHandler(onModeButtonEvent);
  encoderPrecisionButton.setClickHandler(onPrecisionButtonEvent);
}

// Aggiornamento dello stato dei pulsanti
void evaluateButtons() {
  encoderModeButton.update();
  encoderPrecisionButton.update();
}
```

Gli encoder sono utilizzati per selezionare la modalità operativa (Focus, Countdown, Test Strip) e il livello di precisione. I pulsanti degli encoder permettono di cambiare la modalità di precisione (F-stop o Secondi) e la modalità di Test Strip (Continua o Passo-Passo).

- L'encoder di modalità (`encoderMode`) seleziona tra le tre modalità operative principali.
- L'encoder di precisione (`encoderPrecision`) regola il livello di precisione su una scala di 11 valori.
- Il pulsante di modalità (`encoderModeButton`) cambia la modalità di Test Strip tra Continua e Passo-Passo.
- Il pulsante di precisione (`encoderPrecisionButton`) alterna tra la modalità di precisione F-stop e Secondi.

Le funzioni `evaluateEncoderMod()` e `evaluateEncoderPrec()` leggono le posizioni degli encoder e aggiornano lo stato dell'applicazione di conseguenza. Le funzioni `onModeButtonEvent()` e `onPrecisionButtonEvent()` gestiscono gli eventi dei pulsanti, modificando le rispettive modalità.

La funzione `registerClickHandler()` associa i gestori di eventi ai pulsanti,  mentre `evaluateButtons()` aggiorna lo stato dei pulsanti ad ogni ciclo del loop principale.



### Azioni Specifiche

Le azioni per ciascuna modalità sono implementate in `actions.h`. Ad esempio, per il Test Strip:

```cpp
void testStrip() {
    if (!isActionStarted && isFirstTimePressed) {
        initTestStrip();
        isFirstTimePressed = false;
    }
// ... logica per l'incremento del tempo e il controllo del relè
}
```

## Considerazioni sulla Manutenzione

- La struttura modulare facilita l'aggiunta di nuove funzionalità.
- L'uso di funzioni callback permette una facile personalizzazione del comportamento.
- La gestione dello stato attraverso `operationStatus` centralizza il controllo del flusso dell'applicazione.

## Possibili Miglioramenti

1. Implementare un sistema di logging per il debug.
2. Aggiungere la possibilità di salvare e caricare configurazioni personalizzate.
3. Migliorare la gestione degli errori e implementare un sistema di recovery.
4. Ottimizzare l'uso della memoria, specialmente per le stringhe costanti.

