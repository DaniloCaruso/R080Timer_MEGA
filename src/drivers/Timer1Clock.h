// Timer1 in CTC a 1 kHz: la ISR fa avanzare il TimingCore e spegne il relè al
// tick esatto. I comandi dal main sono atomici.
#pragma once

#include <Hal.h>

class Timer1Clock : public TimingPort {
 public:
  void begin();  // chiamare per primo in setup(): mette il relè in uscita a livello basso
  void startCountdown(uint32_t durationMs) override;
  void cancel() override;
  bool running() override;
  uint32_t remainingMs() override;
  uint8_t takeEvents() override;
};
