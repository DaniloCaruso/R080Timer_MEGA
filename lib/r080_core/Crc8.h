// CRC8, polinomio 0x07: chiude ogni blocco EEPROM (TECHNICAL C-10).
#pragma once

#include <stdint.h>

inline uint8_t crc8(const uint8_t* p, uint8_t n) {
  uint8_t crc = 0;
  while (n--) {
    crc ^= *p++;
    for (uint8_t i = 0; i < 8; i++) crc = crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1;
  }
  return crc;
}
