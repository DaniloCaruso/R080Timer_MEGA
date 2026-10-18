// Interfacce verso l'hardware usate dal dominio. Le implementano i driver in
// src/drivers/ e i finti in test/fakes/.
#pragma once

#include <stdint.h>

struct SevenSeg {
  virtual void showDeciseconds(uint16_t ds) = 0;
};

// Caratteri personalizzati che il driver LCD carica all'inizializzazione.
constexpr char LCD_NOTE = 1, LCD_PLUG = 2;

// LCD 16x2. writeAt scrive n caratteri contigui in una sola transazione e
// ritorna false se non sono arrivati. online() è falso dopo un errore del bus,
// finché il driver non ha reinizializzato il display (contenuto da ridisegnare).
struct Lcd {
  virtual bool writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) = 0;
  virtual bool online() = 0;
};

// Log degli eventi: una riga di testo per evento, mai bloccante.
struct EventLog {
  virtual void line(const char* text) = 0;
};

// Comandi al TimingCore della ISR. Il driver li esegue in modo atomico e
// comanda il relè: acceso allo start, spento all'Annullamento.
struct TimingPort {
  virtual void startCountdown(uint32_t durationMs) = 0;
  virtual void cancel() = 0;
  virtual bool running() = 0;
  virtual uint32_t remainingMs() = 0;
};
