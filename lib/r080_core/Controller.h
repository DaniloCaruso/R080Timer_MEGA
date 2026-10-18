// Macchina a stati dell'interfaccia: riceve gli input dell'utente, comanda il
// TimingCore e aggiorna i display.
#pragma once

#include <stdint.h>
#include "Hal.h"
#include "Settings.h"
#include "SoundScheduler.h"

enum class Input : uint8_t { Digit, Star, Hash, StarHold, HashHold, Channel, KeyD, ModeTurn, ModeClick, PrecTurn, PrecClick, MainPress };

class Controller {
 public:
  Controller(TimingPort& timing, SevenSeg& seg, SoundScheduler& sound, Settings& settings)
      : timing_(timing), seg_(seg), sound_(sound), set_(settings) {}
  // arg: la cifra per Digit, il Canale 0..2 per Channel, il verso (+1/-1) per ModeTurn e PrecTurn
  void onInput(Input in, int8_t arg);
  void update();  // a ogni giro di loop

  // Stato letto da Screen.
  bool muted() const { return set_.d.g.mute; }
  uint8_t channelIndex() const { return set_.d.g.activeChannel; }
  const ChannelRecord& channel() const { return set_.active(); }
  uint16_t stripStep() const { return stripShown_; }  // Step in corso da 1, 0 fuori dal test strip

 private:
  void beep(Sound s);
  uint32_t stripStepDs(uint16_t k) const;
  void nextStripStep();

  TimingPort& timing_;
  SevenSeg& seg_;
  SoundScheduler& sound_;
  Settings& set_;  // il Canale attivo: tempo, Tempo base, Offset (-99..+99), precisione, Modalità
  uint16_t shownDs_ = 0xFFFF;
  bool entering_ = false;  // falso: la prossima cifra inizia un tempo nuovo
  uint16_t stripK_ = 0;     // indice da 0 del prossimo Step del test strip
  uint32_t stripDs_ = 0;    // tempo cumulativo del prossimo Step
  uint16_t stripShown_ = 0;

  void stepFStop(ChannelRecord& c, int8_t dir);
};
