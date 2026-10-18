// Clock simulato: contiene il TimingCore vero, tiene lo stato del relè e fa
// avanzare il tempo tick per tick come la ISR di Timer1.
#pragma once
#include <Hal.h>
#include <TimingCore.h>

struct SimClock : TimingPort {
  TimingCore core;
  bool relay = false;
  uint32_t nowMs = 0;
  uint32_t relayOnAtMs = 0, relayOffAtMs = 0;

  void startCountdown(uint32_t durationMs, bool lamp) override {
    core.startCountdown(durationMs);
    relay = lamp;
    if (lamp) relayOnAtMs = nowMs;
  }
  void startFocus(uint32_t limitMs, bool lamp) override {
    core.startFocus(limitMs);
    relay = lamp;
    if (lamp) relayOnAtMs = nowMs;
  }
  void startStrip(bool stepped, uint32_t elapsedMs, uint32_t nextStepMs, bool lamp) override {
    core.startStrip(stepped, elapsedMs, nextStepMs);
    relay = lamp;
    if (lamp) relayOnAtMs = nowMs;
  }
  void setNextStep(uint32_t nextStepMs) override { core.setNextStep(nextStepMs); }
  void cancel() override { core.cancel(); relay = false; relayOffAtMs = nowMs; }
  bool running() override { return core.run() != Run::Idle; }
  uint32_t remainingMs() override { return core.remainingMs(); }
  uint32_t elapsedMs() override { return core.elapsedMs(); }
  uint8_t takeEvents() override { uint8_t e = events; events = 0; return e; }
  void setCorrection(int16_t ppm) override { core.setCorrection(ppm); }
  uint32_t tickCount() override { return nowMs; }
  void setRelay(bool on) override { relay = on; }

  void tick() {
    nowMs++;
    TickOut out = core.tick();
    events |= out.events;
    if (out.relayOff) { relay = false; relayOffAtMs = nowMs; }
  }

 private:
  uint8_t events = 0;
};
