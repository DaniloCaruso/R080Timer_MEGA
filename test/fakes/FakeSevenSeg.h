#pragma once
#include <Hal.h>

struct FakeSevenSeg : SevenSeg {
  uint16_t shown = 0xFFFF;  // nessun valore ancora inviato
  uint32_t writes = 0;
  uint8_t level = 0;  // nessun livello ancora inviato
  void showDeciseconds(uint16_t ds) override { shown = ds; writes++; }
  void setBrightness(uint8_t l) override { level = l; }
};
