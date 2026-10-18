// Immagine voluta dell'LCD (2x16): la confronta con quella già inviata e manda
// solo le differenze, al massimo una riga per giro di loop.
#pragma once

#include <stdint.h>
#include "Controller.h"
#include "Hal.h"

class Screen {
 public:
  Screen(Lcd& lcd, EventLog& log, const Controller& ctl, const char* version)
      : lcd_(lcd), log_(log), ctl_(ctl), version_(version) {}
  void render(uint32_t nowMs);  // a ogni giro di loop
  void showWatchdogReset() { watchdogReset_ = true; }  // all'avvio, al posto del nome (REQ-9.9)

 private:
  void compose(uint32_t nowMs);

  Lcd& lcd_;
  EventLog& log_;
  const Controller& ctl_;
  const char* version_;
  char want_[2][16];
  char shown_[2][16] = {};  // '\0' = cella sconosciuta, da riscrivere
  bool online_ = true;
  bool watchdogReset_ = false;
};
