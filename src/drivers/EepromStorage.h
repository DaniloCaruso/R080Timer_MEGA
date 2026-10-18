// EEPROM interna: solo eeprom_update_*, che riscrive una cella solo se cambia.
#pragma once

#include <Hal.h>

class EepromStorage : public Storage {
 public:
  void read(uint16_t addr, uint8_t* dst, uint16_t n) override;
  void update(uint16_t addr, const uint8_t* src, uint16_t n) override;
};
