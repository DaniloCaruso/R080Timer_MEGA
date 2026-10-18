// Correzione del clock (TECHNICAL C-7): accumulatore in parti per milione, un passo per tick.
#pragma once

#include <stdint.h>

constexpr int16_t PPM_LIMIT = 1000;  // REQ-1.7

class ClockCorrection {
 public:
  void set(int16_t ppm) { ppm_ = ppm; }  // in [-PPM_LIMIT, PPM_LIMIT]
  // Quanti ms contare in questo tick: 0, 1 o 2. Solo somme e confronti: gira nella ISR.
  uint8_t step() {
    acc_ += ppm_;
    if (acc_ >= 1000000L) {
      acc_ -= 1000000L;
      return 2;
    }
    if (acc_ <= -1000000L) {
      acc_ += 1000000L;
      return 0;
    }
    return 1;
  }

 private:
  int32_t acc_ = 0;
  int16_t ppm_ = 0;
};

// Nuova correzione da un Countdown di nominalMs durato measuredMs con la correzione currentPpm:
// (1 + cur/1e6) · measured/nominal − 1, in ppm, limitata a ±PPM_LIMIT (REQ-9.12).
int16_t ppmFromMeasurement(uint32_t nominalMs, uint32_t measuredMs, int16_t currentPpm);
