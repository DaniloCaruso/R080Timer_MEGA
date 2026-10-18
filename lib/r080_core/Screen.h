// Immagine voluta dell'LCD (2x16): la confronta con quella già inviata e manda
// solo le differenze, al massimo una riga per giro di loop.
#pragma once

#include <stdint.h>
#include "Hal.h"

class Screen {
 public:
  Screen(Lcd& lcd, EventLog& log, const char* version) : lcd_(lcd), log_(log), version_(version) {}
  void render(uint32_t nowMs);  // a ogni giro di loop

 private:
  void compose(uint32_t nowMs);

  Lcd& lcd_;
  EventLog& log_;
  const char* version_;
  char want_[2][16];
  char shown_[2][16] = {};  // '\0' = cella sconosciuta, da riscrivere
  bool online_ = true;
};
