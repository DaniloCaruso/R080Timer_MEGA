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

  void startCountdown(uint32_t durationMs) override {
    core.startCountdown(durationMs);
    relay = true;
    relayOnAtMs = nowMs;
  }
  void cancel() override { core.cancel(); relay = false; relayOffAtMs = nowMs; }
  bool running() override { return core.run() != Run::Idle; }
  uint32_t remainingMs() override { return core.remainingMs(); }

  void tick() {
    nowMs++;
    if (core.tick()) { relay = false; relayOffAtMs = nowMs; }
  }
};
