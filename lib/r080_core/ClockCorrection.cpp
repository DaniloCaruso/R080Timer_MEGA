#include "ClockCorrection.h"

int16_t ppmFromMeasurement(uint32_t nominalMs, uint32_t measuredMs, int16_t currentPpm) {
  // float basta: 24 bit di mantissa danno meno di 0,1 ppm di errore.
  float ppm = (1e6f + currentPpm) * (float(measuredMs) / float(nominalMs)) - 1e6f;
  if (ppm >= PPM_LIMIT) return PPM_LIMIT;
  if (ppm <= -PPM_LIMIT) return -PPM_LIMIT;
  return int16_t(ppm < 0 ? ppm - 0.5f : ppm + 0.5f);
}
