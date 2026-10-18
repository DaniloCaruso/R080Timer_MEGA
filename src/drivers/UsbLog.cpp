#include "UsbLog.h"

#include <Arduino.h>

void UsbLog::begin() { Serial.begin(115200); }

void UsbLog::line(const char* text) {
  // shortcut: le righe che non entrano si scartano senza contarle; il contatore arriva con il ticket del log.
  if (Serial.availableForWrite() < static_cast<int>(strlen(text)) + 2) return;
  Serial.println(text);
}
