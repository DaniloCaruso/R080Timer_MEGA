// Contatori persistenti per la diagnostica, in un blocco EEPROM separato dalle impostazioni
// (TECHNICAL C-10): 'R','C', versione, lunghezza | CounterRecord | crc8 @0x040.
#pragma once

#include <stdint.h>
#include "Hal.h"

struct CounterRecord {
  uint16_t powerOns, watchdogResets;
  uint32_t exposures, lampSeconds;
};
static_assert(sizeof(CounterRecord) == 12, "layout EEPROM cambiato: aumentare la versione");

class Counters {
 public:
  explicit Counters(Storage& storage) : storage_(storage) {}
  void boot(bool watchdogReset);  // all'avvio, in setup(): salva subito
  // REQ-9.11: a fine Esposizione, nel loop (mai nella ISR); il salvataggio lo fa service().
  void exposureEnd(bool exposure, uint32_t lampMs);
  void service() { if (writing_) writing_ = !saveStep(); }  // a ogni giro di loop

  CounterRecord d;

 private:
  bool saveStep();  // un byte cambiato per chiamata; true a blocco salvato
  Storage& storage_;
  bool writing_ = false;
  uint16_t lampMsRest_ = 0;  // frazione di secondo non ancora sommata a lampSeconds
};
