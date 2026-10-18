// Timer1 in CTC a 1 kHz: la ISR fa avanzare il TimingCore e spegne il relè al
// tick esatto. I comandi dal main sono atomici.
#pragma once

#include <Hal.h>
#include <ServiceMenu.h>

class Timer1Clock : public TimingPort {
 public:
  void begin();  // chiamare per primo in setup(): mette il relè in uscita a livello basso
  void startCountdown(uint32_t durationMs, bool lamp) override;
  void startFocus(uint32_t limitMs, bool lamp) override;
  void startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs, bool lamp) override;
  void setNextStep(uint32_t nextStepMs) override;
  void cancel() override;
  bool running() override;
  uint32_t remainingMs() override;
  uint32_t elapsedMs() override;
  uint8_t takeEvents() override;
  void setCorrection(int16_t ppm) override;
  uint32_t tickCount() override;
  void setRelay(bool on) override;
  // A ogni giro di loop: durata massima della ISR e tick persi rispetto a Timer4 (REQ-9.4).
  void measure(Health& h);

 private:
  uint16_t lastT4_ = 0;
  uint32_t refCounts_ = 0, expected_ = 0;  // conteggi di Timer4 non ancora tradotti in tick; tick attesi
};
