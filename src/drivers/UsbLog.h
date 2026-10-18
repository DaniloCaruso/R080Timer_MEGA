// Log degli eventi sulla USB a 115200 baud, una riga `t=<ms> <evento>` per evento. Scrive solo
// se la riga entra nel buffer di trasmissione: non blocca mai il loop.
#pragma once

#include <Hal.h>

class UsbLog : public EventLog {
 public:
  void begin();
  void line(const char* text) override;

 private:
  uint16_t lost_ = 0;  // righe scartate non ancora segnalate
};
