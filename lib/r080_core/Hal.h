// Interfacce verso l'hardware usate dal dominio. Le implementano i driver in
// src/drivers/ e i finti in test/fakes/.
#pragma once

#include <stdint.h>

struct SevenSeg {
  virtual void showDeciseconds(uint16_t ds) = 0;
};

// Comandi al TimingCore della ISR. Il driver li esegue in modo atomico e
// comanda il relè: acceso allo start, spento all'Annullamento.
struct TimingPort {
  virtual void startCountdown(uint32_t durationMs) = 0;
  virtual void cancel() = 0;
  virtual bool running() = 0;
  virtual uint32_t remainingMs() = 0;
};
