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
    TEST_ASSERT_FALSE_MESSAGE(core.tick(), "relè spento prima del tempo");
  }
  TEST_ASSERT_EQUAL_UINT32(1, core.remainingMs());
  TEST_ASSERT_TRUE(core.tick());
  TEST_ASSERT_EQUAL_UINT32(0, core.remainingMs());
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  TEST_ASSERT_FALSE(core.tick());
}

static void test_cancelled_countdown_never_turns_relay_off_later() {
  TimingCore core;
  core.startCountdown(100);
  for (int t = 0; t < 50; t++) core.tick();
  core.cancel();
  TEST_ASSERT_TRUE(core.run() == Run::Idle);
  for (int t = 0; t < 100; t++) TEST_ASSERT_FALSE(core.tick());
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_countdown_turns_relay_off_exactly_at_last_tick);
  RUN_TEST(test_cancelled_countdown_never_turns_relay_off_later);
  return UNITY_END();
}
