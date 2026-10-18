#include "Counters.h"

#include <string.h>
#include "Crc8.h"

constexpr uint16_t ADDR = 0x040;
constexpr uint8_t VERSION = 1;
constexpr uint8_t HEADER = 4, BLOCK_LEN = HEADER + sizeof(CounterRecord) + 1;  // 17 byte

// Un blocco non valido riparte da zero: i contatori servono solo alla diagnostica.
void Counters::boot(bool watchdogReset) {
  uint8_t b[BLOCK_LEN];
  storage_.read(ADDR, b, BLOCK_LEN);
  memcpy(&d, b + HEADER, sizeof d);
  bool ok = b[0] == 'R' && b[1] == 'C' && b[2] == VERSION && b[3] == BLOCK_LEN &&
            crc8(b, BLOCK_LEN - 1) == b[BLOCK_LEN - 1];
  if (!ok) d = {};
  // REQ-9.3: le accensioni contano ogni avvio; i reset da watchdog hanno anche il loro contatore.
  d.powerOns++;
  if (watchdogReset) d.watchdogResets++;
  while (!saveStep()) {}  // in setup() l'attesa è ammessa: al più 17 byte, meno di 60 ms
}

void Counters::exposureEnd(bool exposure, uint32_t lampMs) {
  if (exposure) d.exposures++;
  lampMs += lampMsRest_;
  d.lampSeconds += lampMs / 1000;
  lampMsRest_ = lampMs % 1000;
  writing_ = true;
}

bool Counters::saveStep() {
  uint8_t b[BLOCK_LEN] = {'R', 'C', VERSION, BLOCK_LEN};
  memcpy(b + HEADER, &d, sizeof d);
  b[BLOCK_LEN - 1] = crc8(b, BLOCK_LEN - 1);
  return writeBlockStep(storage_, ADDR, b, BLOCK_LEN);
}
