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
  void boot(bool watchdogReset);  // all'avvio

  CounterRecord d;

 private:
  Storage& storage_;
};
