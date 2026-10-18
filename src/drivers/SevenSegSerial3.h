// 7 segmenti seriale su Serial3 (TX3 = D14). Il buffer di trasmissione
// hardware da 64 byte rende le scritture non bloccanti.
#pragma once

#include <Hal.h>

class SevenSegSerial3 : public SevenSeg {
 public:
  void begin();
  void showDeciseconds(uint16_t ds) override;
  void setBrightness(uint8_t level) override;
};
