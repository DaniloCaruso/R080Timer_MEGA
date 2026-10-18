// LCD Waveshare 1602 RGB su I²C (controller LCD a 0x3E, retroilluminazione PCA9633
// a 0x60), senza attese: timeout I²C, una transazione per sequenza di caratteri,
// inizializzazione a passi temporizzati e nuovo tentativo ogni 2 s dopo un errore.
#pragma once

#include <Hal.h>

class LcdWaveshare : public Lcd {
 public:
  void begin();
  bool writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) override;
  bool online() override;  // a ogni giro: fa avanzare l'inizializzazione

 private:
  bool initStep();
  void fail();

  uint8_t step_ = 0;      // passo dell'inizializzazione, READY quando finita
  uint32_t nextMs_ = 50;  // il controller vuole 40 ms dall'accensione prima dei comandi
  bool online_ = true;    // falso dal primo errore fino alla prossima inizializzazione riuscita
};
