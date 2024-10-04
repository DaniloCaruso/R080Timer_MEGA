# R080 TIMER MEGA - Timer per Ingranditore Fotografico

## Descrizione
Questo progetto implementa un timer digitale avanzato per ingranditore fotografico, basato su Arduino Mega 2560. Il dispositivo offre diverse modalità operative, tra cui Focus, Countdown e Test Strip, con controlli precisi per la gestione dei tempi di esposizione.

## Scheda di Sviluppo

Questo progetto è stato sviluppato specificamente per l'**Arduino Mega 2560**. 

### Caratteristiche principali dell'Arduino Mega 2560:
- Microcontrollore: ATmega2560
- Tensione operativa: 5V
- Pin digitali I/O: 54 (di cui 15 possono essere usati come uscite PWM)
- Pin analogici in ingresso: 16
- Corrente DC per pin I/O: 20 mA
- Corrente DC per pin 3.3V: 50 mA
- Memoria Flash: 256 KB (di cui 8 KB usati dal bootloader)
- SRAM: 8 KB
- EEPROM: 4 KB
- Clock: 16 MHz

La scelta dell'Arduino Mega 2560 è stata motivata dalla necessità di un elevato numero di pin I/O per gestire tutti i componenti del progetto (display, encoder, tastierino, ecc.) e dalla maggiore capacità di memoria rispetto ad altre schede Arduino, che permette di implementare funzionalità più complesse.

## Configurazione Hardware

### Componenti utilizzati:
1. Arduino Mega 2560
2. Display LCD 16x2 (I2C)
3. Display a 7 segmenti
4. Tastierino numerico 4x4
5. 2x Encoder rotativi con pulsante
6. Pulsante principale
7. Buzzer
8. Relè per il controllo dell'ingranditore

### Collegamenti:


- **LCD I2C**: 
  - SDA -> Pin 20
  - SCL -> Pin 21

- **Display 7 segmenti**:
  - DIN -> Pin A2

- **Tastierino 4x4**:
  - Righe: Pin 33, 31, 29, 27
  - Colonne: Pin 41, 39, 37, 35

- **Encoder Modalità**:
  - DT -> Pin 3
  - CLK -> Pin 4
  - SW -> Pin 17

- **Encoder Precisione**:
  - DT -> Pin 18
  - CLK -> Pin 19
  - SW -> Pin 16

- **Pulsante Principale**: Pin 2

- **Buzzer**: Pin 49

- **Relè Ingranditore**: Pin 47

## Configurazione Software

### Ambiente di sviluppo
Questo progetto utilizza PlatformIO come ambiente di sviluppo. Per configurare il progetto:

- #### Utilizzando Visual Studio Code:
  1. Installare l'estensione PlatformIO IDE per Visual Studio Code
  2. Clonare questo repository
  3. Aprire la cartella del progetto in Visual Studio Code
  4. Cliccare sull'icona PlatformIO nella barra laterale
  5. Selezionare "Build" per compilare il progetto
  6. Selezionare "Upload" per caricare il firmware su Arduino Mega 2560

- #### Utilizzando PlatformIO Core:

  1. Installare [PlatformIO Core](https://docs.platformio.org/en/latest/core/installation.html)
  2. Clonare questo repository
  3. Aprire una finestra di comando nella directory del progetto
  4. Eseguire `pio run` per compilare il progetto
  5. Eseguire `pio run --target upload` per caricare il firmware su Arduino Mega 2560

### Librerie utilizzate

- **EncoderButton**: Gestione di encoder rotativi con pulsante integrato
  - [Github](https://github.com/Stutchbury/EncoderButton)

- **Encoder**: Gestione di encoder rotativi
  - [Github](https://github.com/PaulStoffregen/Encoder)

- **Keypad**: Gestione del tastierino numerico
  - [Github](https://playground.arduino.cc/Code/Keypad/)
  
- **Waveshare_LCD1602_RGB**: Controllo del display LCD RGB tramite I2C
  - [Wiki](https://www.waveshare.com/wiki/LCD1602_RGB_Module)

- **ArduinoSTL**: Implementazione di alcune funzionalità della libreria standard C++
  - [Github](https://github.com/mike-matera/ArduinoSTL)


## Contribuire

Le pull request sono benvenute. 
Per modifiche importanti, aprite prima un issue per discutere cosa vorreste cambiare.

## Licenza

[CC0 1.0 Universal](LICENSE)

