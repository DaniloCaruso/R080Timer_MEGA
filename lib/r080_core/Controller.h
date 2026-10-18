// Macchina a stati dell'interfaccia: riceve gli input dell'utente, comanda il
// TimingCore e aggiorna i display.
#pragma once

#include <stdint.h>
#include "Hal.h"
#include "SoundScheduler.h"

enum class Input : uint8_t { Digit, Star, Hash, StarHold, PrecTurn, PrecClick, MainPress };

class Controller {
 public:
  Controller(TimingPort& timing, SevenSeg& seg, SoundScheduler& sound)
      : timing_(timing), seg_(seg), sound_(sound) {}
  void onInput(Input in, int8_t arg);  // arg: la cifra per Digit, il verso (+1/-1) per PrecTurn
  void update();                       // a ogni giro di loop
  bool muted() const { return muted_; }

  // Stato letto da Screen.
  uint8_t precisionIndex() const { return precIdx_; }
  bool fstop() const { return fstop_; }
  int8_t offset() const { return offset_; }

 private:
  void beep(Sound s);

  TimingPort& timing_;
  SevenSeg& seg_;
  SoundScheduler& sound_;
  uint16_t timeDs_ = 100;  // 10,0 s, il valore di fabbrica
  uint16_t shownDs_ = 0xFFFF;
  bool entering_ = false;  // falso: la prossima cifra inizia un tempo nuovo
  bool muted_ = false;
  uint8_t precIdx_ = 0;    // indice nelle liste di Precision.h
  bool fstop_ = false;     // tipo di precisione: falso = Secondi
  uint16_t baseDs_ = 100;  // Tempo base
  int8_t offset_ = 0;      // Offset in frazioni di stop, -99..+99

  void stepFStop(int8_t dir);
};
