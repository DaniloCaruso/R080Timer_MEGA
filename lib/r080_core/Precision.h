// Liste di precisione (REQ-4.1) e calcolo F-Stop. Calcolo in float: solo nel
// loop principale, mai nella ISR.
#pragma once

#include <math.h>
#include <stdint.h>

constexpr uint8_t PRECISION_COUNT = 11;
constexpr uint8_t kFStopDenominators[PRECISION_COUNT] = {1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 64};
constexpr uint8_t kSecondsSteps[PRECISION_COUNT] = {1, 2, 3, 4, 5, 6, 7, 8, 10, 15, 20};

// Tempo base·2^(offset/denom) arrotondato al decimo, senza limiti (satura a 65535):
// chi chiama confronta con DS_MIN..DS_MAX.
inline uint16_t fstopTime(uint16_t baseDs, int16_t offset, uint8_t denom) {
  float t = baseDs * powf(2.f, static_cast<float>(offset) / denom) + 0.5f;  // avr-libc non ha exp2f
  return t >= 65535.f ? 65535 : static_cast<uint16_t>(t);
}
