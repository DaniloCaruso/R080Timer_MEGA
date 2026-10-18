#pragma once
#include <string.h>
#include <Hal.h>

// Log finto: conta le righe, ricorda l'ultima e tiene tutto il testo, una riga per '\n'.
struct FakeLog : EventLog {
  char last[48] = "";
  char text[2048] = "";
  uint32_t lines = 0;
  void line(const char* t) override {
    strncpy(last, t, sizeof last - 1);
    lines++;
    if (strlen(text) + strlen(t) + 2 <= sizeof text) strcat(strcat(text, t), "\n");
  }
  // Righe uguali a t.
  uint8_t count(const char* t) const {
    uint8_t n = 0;
    size_t len = strlen(t);
    for (const char* p = text; *p; p = strchr(p, '\n') + 1)
      if (!strncmp(p, t, len) && p[len] == '\n') n++;
    return n;
  }
};
