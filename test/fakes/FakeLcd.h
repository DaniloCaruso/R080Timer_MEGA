#pragma once
#include <string.h>
#include <Hal.h>

// LCD 16x2 finto: tiene il contenuto cella per cella e conta le scritture.
struct FakeLcd : Lcd {
  char cells[2][17];  // ogni riga terminata da '\0' per i confronti tra stringhe
  bool isOnline = true;
  uint32_t writes = 0;

  FakeLcd() { fill(' '); }
  void fill(char c) {
    for (auto& row : cells) { memset(row, c, 16); row[16] = '\0'; }
  }
  bool writeAt(uint8_t col, uint8_t row, const char* s, uint8_t n) override {
    if (!isOnline || row > 1 || col + n > 16) return false;
    memcpy(&cells[row][col], s, n);
    writes++;
    return true;
  }
  bool online() override { return isOnline; }
};
