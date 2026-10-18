// Interfacce verso l'hardware usate dal dominio. Le implementano i driver in
// src/drivers/ e i finti in test/fakes/.
#pragma once

#include <stdint.h>

// Luminosità a livelli 1..LEVEL_COUNT: i driver li traducono nei valori del dispositivo (REQ-7.11).
constexpr uint8_t LEVEL_COUNT = 8;

struct SevenSeg {
  virtual void showDeciseconds(uint16_t ds) = 0;
  virtual void setBrightness(uint8_t level) = 0;
};

// Caratteri personalizzati che il driver LCD carica all'inizializzazione.
constexpr char LCD_NOTE = 1, LCD_PLUG = 2;

// LCD 16x2. writeAt scrive n caratteri contigui in una sola transazione e
// ritorna false se non sono arrivati. online() è falso dopo un errore del bus,
// finché il driver non ha reinizializzato il display (contenuto da ridisegnare).
// setBrightness vale anche per le reinizializzazioni successive.
struct Lcd {
  virtual bool writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) = 0;
  virtual bool online() = 0;
  virtual void setBrightness(uint8_t level) = 0;
};

// Buzzer: suona hz per ms e si ferma da solo; una nuova nota sostituisce quella in corso.
struct Buzzer {
  virtual void play(uint16_t hz, uint16_t ms) = 0;
};

// EEPROM: update() riceve solo i byte da scrivere.
struct Storage {
  virtual void read(uint16_t addr, uint8_t* dst, uint16_t n) = 0;
  virtual void update(uint16_t addr, const uint8_t* src, uint16_t n) = 0;
};

// Log degli eventi: una riga di testo per evento, mai bloccante.
struct EventLog {
  virtual void line(const char* text) = 0;
};

// Comandi al TimingCore della ISR. Il driver li esegue in modo atomico e
// comanda il relè: acceso allo start se lamp è vero, spento all'Annullamento.
// startStrip riprende anche il passo-passo in pausa, da elapsedMs.
struct TimingPort {
  virtual void startCountdown(uint32_t durationMs, bool lamp) = 0;
  virtual void startFocus(uint32_t limitMs, bool lamp) = 0;
  virtual void startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs, bool lamp) = 0;
  virtual void setNextStep(uint32_t nextStepMs) = 0;
  virtual void cancel() = 0;
  virtual bool running() = 0;
  virtual uint32_t remainingMs() = 0;
  virtual uint32_t elapsedMs() = 0;
  virtual uint8_t takeEvents() = 0;  // eventi EV_* accumulati dalla ISR dall'ultima chiamata
  virtual void setRelay(bool on) = 0;  // test del relè nel Menu di servizio, a TimingCore fermo
};
