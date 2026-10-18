// Buzzer su D49 con tone() (Timer2): la nota si ferma da sola alla fine della
// durata, senza attese nel loop.
#pragma once

#include <Hal.h>

class BuzzerTone : public Buzzer {
 public:
  void play(uint16_t hz, uint16_t ms) override;
};
