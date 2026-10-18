// Blocchi EEPROM (TECHNICAL C-10): CRC8, polinomio 0x07, e scrittura a passi.
#pragma once

#include <stdint.h>
#include "Hal.h"

inline uint8_t crc8(const uint8_t* p, uint8_t n) {
  uint8_t crc = 0;
  while (n--) {
    crc ^= *p++;
    for (uint8_t i = 0; i < 8; i++) crc = crc & 0x80 ? (crc << 1) ^ 0x07 : crc << 1;
  }
  return crc;
}

// Un passo del salvataggio del blocco img (len byte, CRC per ultimo) all'indirizzo addr: scrive al più
// un byte diverso dalla EEPROM, e il CRC solo quando tutti gli altri coincidono. Un salvataggio
// interrotto lascia il CRC vecchio e il blocco si scarta al riavvio. Ritorna true a blocco salvato.
inline bool writeBlockStep(Storage& st, uint16_t addr, const uint8_t* img, uint8_t len) {
  if (!st.ready()) return false;  // la scrittura precedente non è finita: niente attese nel loop
  for (uint8_t i = 0; i < len; i++) {
    uint8_t old;
    st.read(addr + i, &old, 1);
    if (old != img[i]) {
      st.update(addr + i, &img[i], 1);
      return false;
    }
  }
  return true;
}
