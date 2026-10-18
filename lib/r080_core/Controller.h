// Macchina a stati dell'interfaccia: riceve gli input dell'utente, comanda il
// TimingCore e aggiorna i display.
#pragma once

#include <stdint.h>
#include "Hal.h"

enum class Input : uint8_t { Digit, MainPress };

class Controller {
 public:
  Controller(TimingPort& timing, SevenSeg& seg) : timing_(timing), seg_(seg) {}
  void onInput(Input in, int8_t arg);  // arg: la cifra per Input::Digit
  void update();                       // a ogni giro di loop

 private:
  TimingPort& timing_;
  SevenSeg& seg_;
  uint16_t timeDs_ = 100;  // 10,0 s, il valore di fabbrica
  uint16_t shownDs_ = 0xFFFF;
  bool entering_ = false;  // falso: la prossima cifra inizia un tempo nuovo
};
