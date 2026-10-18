// Macchina a stati dell'interfaccia: riceve gli input dell'utente, comanda il
// TimingCore e aggiorna i display.
#pragma once

#include <stdint.h>
#include "Counters.h"
#include "Hal.h"
#include "ServiceMenu.h"
#include "Settings.h"
#include "SoundScheduler.h"

enum class Input : uint8_t { Digit, Star, Hash, StarHold, HashHold, ZeroStar, ZeroHash, Channel, KeyD, ModeTurn,
                             ModeClick, PrecTurn, PrecClick, MainPress, MainRelease };

class Controller {
 public:
  Controller(TimingPort& timing, SevenSeg& seg, SoundScheduler& sound, Settings& settings, Counters& counters,
             EventLog& log, ServiceMenu& menu)
      : timing_(timing), seg_(seg), sound_(sound), set_(settings), cnt_(counters), log_(log), menu_(menu) {}
  // All'avvio, dopo Settings::load() e Counters::boot(): riga di avvio nel log e, con `D`
  // tenuto all'accensione, Menu di servizio (REQ-9.1).
  void boot(bool watchdogReset, bool settingsOk, bool keyD);
  // arg: la cifra per Digit, il Canale 0..2 per Channel, il verso (+1/-1) per ModeTurn e PrecTurn
  void onInput(Input in, int8_t arg);
  void update();  // a ogni giro di loop

  // Stato letto da Screen.
  bool muted() const { return set_.d.g.mute; }
  uint8_t channelIndex() const { return set_.d.g.activeChannel; }
  const ChannelRecord& channel() const { return set_.active(); }
  // REQ-7.3: durante l'Esposizione l'LCD scende al livello di esposizione.
  uint8_t lcdLevel() const { return timing_.running() ? set_.d.g.lcdExposureLevel : set_.d.g.lcdLevel; }
  uint16_t stripStep() const { return stripShown_; }  // Step in corso da 1, 0 fuori dal test strip
  const ServiceMenu* service() const { return menu_.active() ? &menu_ : nullptr; }  // aperto o nullptr

 private:
  void beep(Sound s);
  uint32_t stripStepDs(uint16_t k) const;
  void nextStripStep();
  void logStart(const ChannelRecord& c, uint32_t ms);
  void exposureEnd(const char* how);
  void calibrationLog();

  TimingPort& timing_;
  SevenSeg& seg_;
  SoundScheduler& sound_;
  Settings& set_;  // il Canale attivo: tempo, Tempo base, Offset (-99..+99), precisione, Modalità
  Counters& cnt_;
  EventLog& log_;
  ServiceMenu& menu_;
  uint32_t startMs_ = 0;  // durata chiesta all'ultimo start, per il log
  uint16_t shownDs_ = 0xFFFF;
  uint8_t segLevel_ = 0;  // livello già inviato al 7 segmenti
  int16_t ppm_ = 0;       // Correzione del clock già inviata al TimingCore, che parte da 0
  uint32_t calNextMs_ = 0;  // tick della prossima riga del log di calibrazione, 0 a log spento
  bool entering_ = false;  // falso: la prossima cifra inizia un tempo nuovo
  uint16_t stripK_ = 0;     // indice da 0 del prossimo Step del test strip
  uint32_t stripDs_ = 0;    // tempo cumulativo del prossimo Step
  uint16_t stripShown_ = 0;
  bool paused_ = false;     // passo-passo fermo a uno Step, in attesa del pulsante principale

  void stepFStop(ChannelRecord& c, int8_t dir);
};
