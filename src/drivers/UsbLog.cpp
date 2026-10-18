#include "UsbLog.h"

#include <Arduino.h>

void UsbLog::begin() { Serial.begin(115200); }

// Scrive la riga con l'istante in ms, solo se entra tutta nel buffer di trasmissione.
static bool put(const char* text) {
  char b[64];
  int n = snprintf_P(b, sizeof b, PSTR("t=%lu %s"), millis(), text);
  if (Serial.availableForWrite() < n + 2) return false;
  Serial.println(b);
  return true;
}

void UsbLog::line(const char* text) {
  if (lost_) {  // REQ-9.10: le righe scartate si contano e si segnalano appena c'è posto
    char b[24];
    snprintf_P(b, sizeof b, PSTR("LOST lines=%u"), lost_);
    if (put(b)) lost_ = 0;
  }
  if (!put(text)) lost_++;
}
