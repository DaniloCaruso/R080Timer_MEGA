#include "EepromStorage.h"

#include <avr/eeprom.h>

void EepromStorage::read(uint16_t addr, uint8_t* dst, uint16_t n) {
  eeprom_read_block(dst, reinterpret_cast<const void*>(addr), n);
}

void EepromStorage::update(uint16_t addr, const uint8_t* src, uint16_t n) {
  eeprom_update_block(src, reinterpret_cast<void*>(addr), n);
}
