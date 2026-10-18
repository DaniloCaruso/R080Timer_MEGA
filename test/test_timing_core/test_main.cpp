// Seam 2: TimingCore da solo, tick per tick.
#include <unity.h>
#include <TimingCore.h>

void setUp() {}
void tearDown() {}

// REQ-1.2: il relè si spegne esattamente al tick 10000 di un Countdown di 10,0 s.
static void test_countdown_turns_relay_off_exactly_at_last_tick() {
  TimingCore core;
  core.startCountdown(10000);
  for (uint32_t t = 1; t < 10000; t++) {
    TEST_ASSERT_FALSE_MESSAGE(core.tick().relayOff, "relè spento prima del tempo");
  }
  TEST_ASSERT_EQUAL_UINT32(1, core.remainingMs());
  TickOut out = core.tick();
  TEST_ASSERT_TRUE(out.relayOff);
  TEST_ASSERT_EQUAL_UINT8(EV_END, out.events);
  TEST_ASSERT_EQUAL_UINT32(0, core.remainingMs());
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  TEST_ASSERT_FALSE(core.tick().relayOff);
}

static void test_cancelled_countdown_never_turns_relay_off_later() {
  TimingCore core;
  core.startCountdown(100);
  for (int t = 0; t < 50; t++) core.tick();
  core.cancel();
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  for (int t = 0; t < 100; t++) TEST_ASSERT_FALSE(core.tick().relayOff);
}

// REQ-1.6: il Metronomo scatta a ogni secondo intero contato dall'inizio dell'Esposizione.
static void test_metronome_fires_every_second_from_exposure_start() {
  TimingCore core;
  core.startCountdown(3500);
  for (uint32_t t = 1; t < 3500; t++) {
    uint8_t expected = (t == 1000 || t == 2000 || t == 3000) ? EV_METRONOME : 0;
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected, core.tick().events, "Metronomo fuori tempo");
  }
  TEST_ASSERT_EQUAL_UINT8(EV_END, core.tick().events);
}

// REQ-1.6: un nuovo Countdown riparte da zero, anche dopo un Annullamento a metà secondo.
static void test_metronome_restarts_with_each_exposure() {
  TimingCore core;
  core.startCountdown(5000);
  for (int t = 0; t < 1500; t++) core.tick();
  core.cancel();
  core.startCountdown(5000);
  for (uint32_t t = 1; t < 1000; t++) TEST_ASSERT_EQUAL_UINT8(0, core.tick().events);
  TEST_ASSERT_EQUAL_UINT8(EV_METRONOME, core.tick().events);
}

// REQ-8.2: nessun Metronomo da fermo.
static void test_no_metronome_when_idle() {
  TimingCore core;
  for (int t = 0; t < 3000; t++) TEST_ASSERT_EQUAL_UINT8(0, core.tick().events);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_countdown_turns_relay_off_exactly_at_last_tick);
  RUN_TEST(test_cancelled_countdown_never_turns_relay_off_later);
  RUN_TEST(test_metronome_fires_every_second_from_exposure_start);
  RUN_TEST(test_metronome_restarts_with_each_exposure);
  RUN_TEST(test_no_metronome_when_idle);
  return UNITY_END();
}
