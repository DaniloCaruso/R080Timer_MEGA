#pragma once
#include <string.h>
#include <Hal.h>

// EEPROM finta: tiene i byte (0xFF come una EEPROM vergine) e conta quelli passati a update().
struct FakeStorage : Storage {
  uint8_t bytes[128];
  uint32_t written = 0;

  FakeStorage() { erase(); }
  void erase() { memset(bytes, 0xFF, sizeof bytes); written = 0; }
  void read(uint16_t addr, uint8_t* dst, uint16_t n) override { memcpy(dst, &bytes[addr], n); }
  void update(uint16_t addr, const uint8_t* src, uint16_t n) override {
    memcpy(&bytes[addr], src, n);
    written += n;
  }
};
