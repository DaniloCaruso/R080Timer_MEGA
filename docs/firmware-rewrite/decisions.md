# Decisioni — firmware-rewrite (ESP32, MicroPython)

> **In pausa dal 2026-10-26.** Da riprendere: ruolo dell'ESP32 (timer completo oppure solo ponte WiFi/Bluetooth verso il Mega), scelta dei componenti per i 3,3 V (relè BESTEP 3 V, LCD SMH1602A rosso su nero 5 V con PCF8574 e convertitore di livello BSS138, transistor per la retroilluminazione, buzzer), alimentazione.

Riscrittura enterprise-grade del firmware R080 Timer per ESP32-WROOM-32 in MicroPython. Sessione di grilling del 2026-10-26, preceduta dalla code review del port MicroPython esistente (difetti di sicurezza e di comportamento: relè acceso dopo un'eccezione, nessun watchdog, `*`/`#` che non cambiano l'esposizione reale, tempi digitati non salvati, step del test strip che derivano).

## Scopo della versione ESP32

**Decisione:** stesso prodotto della v2.0.0 Arduino, stesse funzioni. WiFi/Bluetooth con un'app Android che gestisce l'intero timer vanno nel backlog.

## Due implementazioni ESP32

**Decisione:** si faranno sia una versione MicroPython sia una versione C++. Si parte da MicroPython su questo branch. La versione C++ vivrà sul branch `feature/esp32-cpp-firmware`, da avviare dopo il collaudo della v2.0.0 Arduino, e potrà riusare la libreria di dominio C++ di Arduino.

## Specifica di comportamento ereditata

**Decisione:** la v2.0.0 Arduino (`docs/firmware-rewrite/` sul branch `feature/arduino-mega-firmware`: EARS, decisions.md, GLOSSARY.md) è la specifica di comportamento anche per l'ESP32. Qui si decidono solo le differenze di piattaforma.

## Hardware

**Decisione:** stessa scheda ESP32-WROOM-32 DevKit 38 pin, stessi componenti, stessa mappa pin del port esistente, più una resistenza da 10 kΩ tra GPIO23 (relè) e GND: GPIO23 resta flottante durante e dopo il reset finché il firmware non lo configura.

## Riscrittura, non correzione

**Decisione:** riscrittura da capo con tre strati: dominio in Python puro (nessun import di `machine`, `esp32`, `uasyncio`), driver sottili, `main.py` che collega. Il port esistente resta come riferimento sotto il tag `esp32-micropython-port-v1`.

## Test e CI

**Decisione:** `pytest` sul PC sulle stesse due seam di Arduino (dominio intero con driver finti; motore del tempo da solo). GitHub Action che a ogni push esegue i test e compila tutti i moduli con `mpy-cross`.

## Caricamento sulla scheda

**Decisione:** moduli precompilati `.mpy` (mpy-cross) caricati con `mpremote` tramite script. I moduli frozen nel firmware vanno nel backlog, da usare solo se la RAM non basta.

## Persistenza NVS

**Decisione:** impostazioni salvate come un unico blob NVS con versione e CRC8 (stesso formato logico del Mega), contatori in un blob separato, salvataggio ritardato di 2 s come su Arduino.

## Branch e documenti

**Decisione:** si lavora su `feature/esp32-micropython-firmware`; documenti in `docs/firmware-rewrite/` su questo branch (EARS, TECHNICAL, MANAGEMENT ricavati da quelli Arduino e adattati). Issue con etichetta `esp32`, in inglese.

## Spegnimento preciso del relè

**Contesto:** su ESP32 MicroPython v1.29.0 i callback di `machine.Timer` sono sempre soft (schedulati), con ritardi possibili di decine di ms per GC, scritture flash o task di sistema; `hard=True` non è implementato. L'unica periferica che genera un fronte preciso indipendente dall'interprete è l'RMT (singola durata fino a circa 105 ms).
**Decisione:** Python accende il relè e registra l'istante in µs; circa 200 ms prima della fine un timer soft calcola il tempo residuo e affida il pin all'RMT, che tiene il relè acceso per quel residuo e lo spegne in hardware. Vale per Countdown, step del passo-passo e Spegnimento automatico. Il primo ticket è un prototipo sulla scheda che misura il fronte. Ripiego documentato se il prototipo fallisce: timer soft con GC disabilitato durante l'esposizione e nessuna operazione lenta (flash, NVS) in quel periodo. Il requisito resta ±1 ms, verificato sulla scheda.

## Watchdog e sicurezza del relè

**Decisione:** `machine.WDT` a 1 s (minimo su ESP32, non si può fermare). In più: relè spento in un blocco che intercetta qualunque eccezione di Python, pull-down da 10 kΩ su GPIO23 per l'avvio, RMT come ulteriore garanzia nell'ultimo tratto dell'esposizione. Un reset da calo di tensione non è distinguibile da un'accensione (`reset_cause()` lo riporta come `PWRON_RESET`).

## Versione di MicroPython

**Decisione:** firmware MicroPython fissato alla v1.29.0. Lo script di flash installa quella versione; all'avvio il firmware verifica la versione e, se diversa, non parte e lo segnala sull'LCD.
