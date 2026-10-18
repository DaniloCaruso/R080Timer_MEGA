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

// REQ-3.3, REQ-3.4: in Focus l'avviso a limite -10 s e lo Spegnimento automatico al limite esatto.
static void test_focus_warns_ten_seconds_before_limit_and_turns_relay_off_at_limit() {
  TimingCore core;
  core.startFocus(300000);
  for (uint32_t t = 1; t < 300000; t++) {
    TickOut out = core.tick();
    TEST_ASSERT_FALSE_MESSAGE(out.relayOff, "relè spento prima del limite");
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(t == 290000 ? EV_FOCUS_WARN : 0, out.events, "evento fuori tempo");
  }
  TEST_ASSERT_EQUAL_UINT32(299999, core.elapsedMs());
  TickOut out = core.tick();
  TEST_ASSERT_TRUE(out.relayOff);
  TEST_ASSERT_EQUAL_UINT8(EV_FOCUS_OFF, out.events);
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  TEST_ASSERT_FALSE(core.tick().relayOff);
}

static void test_cancelled_focus_never_turns_relay_off_later() {
  TimingCore core;
  core.startFocus(180000);
  for (int t = 0; t < 1000; t++) core.tick();
  TEST_ASSERT_EQUAL_UINT32(1000, core.elapsedMs());
  core.cancel();
  for (uint32_t t = 0; t < 180000; t++) TEST_ASSERT_EQUAL_UINT8(0, core.tick().events);
}

// REQ-1.5, REQ-8.3: Step al tick esatto, Preavviso a -300, -200, -100 ms; dopo lo Step il
// prossimo arriva solo quando il loop lo ha calcolato.
static void test_strip_steps_at_exact_tick_with_prewarn() {
  TimingCore core;
  core.startStrip(false, 0, 1000);
  for (uint32_t t = 1; t <= 3000; t++) {
    if (t == 1100) core.setNextStep(1500);
    uint8_t expected = (t == 700 || t == 800 || t == 900 || t == 1200 || t == 1300 || t == 1400) ? EV_PREWARN
                       : (t == 1000 || t == 1500)                                                 ? EV_STEP
                                                                                                  : 0;
    TickOut out = core.tick();
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(expected, out.events, "evento fuori tempo");
    TEST_ASSERT_FALSE(out.relayOff);
  }
  TEST_ASSERT_EQUAL_UINT32(3000, core.elapsedMs());
  TEST_ASSERT_TRUE(core.run() == Run::StripContinuous);
}

// REQ-3.14: il test strip si conclude a 999,9 s esatti con relè spento ed EV_END.
static void test_strip_ends_at_999_9_seconds() {
  TimingCore core;
  core.startStrip(false, 0, 2000000);  // nessuno Step prima del limite
  for (uint32_t t = 1; t < 999900; t++) {
    if (core.tick().relayOff) TEST_FAIL_MESSAGE("relè spento prima del limite");
  }
  TickOut out = core.tick();
  TEST_ASSERT_TRUE(out.relayOff);
  TEST_ASSERT_EQUAL_UINT8(EV_END, out.events);
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  TEST_ASSERT_EQUAL_UINT8(0, core.tick().events);
}

// REQ-1.3, REQ-3.12: in passo-passo il relè si spegne al tick esatto dello Step e il core si
// ferma; la ripresa riparte dal tempo cumulativo e conta solo l'incremento.
static void test_stepped_strip_turns_relay_off_at_exact_step_tick() {
  TimingCore core;
  core.startStrip(true, 0, 1000);
  for (uint32_t t = 1; t < 1000; t++) TEST_ASSERT_FALSE_MESSAGE(core.tick().relayOff, "relè spento prima dello Step");
  TickOut out = core.tick();
  TEST_ASSERT_TRUE(out.relayOff);
  TEST_ASSERT_EQUAL_UINT8(EV_STEP, out.events);
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  for (int t = 0; t < 5000; t++) {
    out = core.tick();
    TEST_ASSERT_FALSE(out.relayOff);
    TEST_ASSERT_EQUAL_UINT8(0, out.events);
  }
  TEST_ASSERT_EQUAL_UINT32(1000, core.elapsedMs());
  core.startStrip(true, 1000, 1500);
  for (uint32_t t = 1; t < 500; t++) TEST_ASSERT_FALSE(core.tick().relayOff);
  out = core.tick();
  TEST_ASSERT_TRUE(out.relayOff);
  TEST_ASSERT_EQUAL_UINT8(EV_STEP, out.events);
  TEST_ASSERT_EQUAL_UINT32(1500, core.elapsedMs());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_focus_warns_ten_seconds_before_limit_and_turns_relay_off_at_limit);
  RUN_TEST(test_cancelled_focus_never_turns_relay_off_later);
  RUN_TEST(test_countdown_turns_relay_off_exactly_at_last_tick);
  RUN_TEST(test_cancelled_countdown_never_turns_relay_off_later);
  RUN_TEST(test_metronome_fires_every_second_from_exposure_start);
  RUN_TEST(test_metronome_restarts_with_each_exposure);
  RUN_TEST(test_no_metronome_when_idle);
  RUN_TEST(test_strip_steps_at_exact_tick_with_prewarn);
  RUN_TEST(test_strip_ends_at_999_9_seconds);
  RUN_TEST(test_stepped_strip_turns_relay_off_at_exact_step_tick);
  return UNITY_END();
}
