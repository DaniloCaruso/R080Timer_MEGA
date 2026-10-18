#pragma once
#include <Hal.h>

struct FakeSevenSeg : SevenSeg {
  uint16_t shown = 0xFFFF;  // nessun valore ancora inviato
  uint32_t writes = 0;
  void showDeciseconds(uint16_t ds) override { shown = ds; writes++; }
};
