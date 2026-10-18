#pragma once
#include <string.h>
#include <Hal.h>

// Log finto: conta le righe e ricorda l'ultima.
struct FakeLog : EventLog {
  char last[32] = "";
  uint32_t lines = 0;
  void line(const char* text) override {
    strncpy(last, text, sizeof last - 1);
    lines++;
  }
};
