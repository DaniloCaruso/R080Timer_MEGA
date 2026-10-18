// Seam 1: dominio intero con il TimingCore vero e driver finti. I test agiscono
// solo con input dell'utente e tempo che passa.
#include <unity.h>
#include <Controller.h>
#include "../fakes/FakeSevenSeg.h"
#include "../fakes/SimClock.h"

struct Timer {
  SimClock clock;
  FakeSevenSeg seg;
  Controller ctl{clock, seg};

  Timer() { ctl.update(); }
  void type(const char* digits) {
    for (; *digits; digits++) { ctl.onInput(Input::Digit, *digits - '0'); ctl.update(); }
  }
  void press() { ctl.onInput(Input::MainPress, 0); ctl.update(); }
  void advance(uint32_t ms) {
    while (ms--) { clock.tick(); ctl.update(); }
  }
};

void setUp() {}
void tearDown() {}

// REQ-5.1
static void test_digits_enter_from_the_right() {
  Timer t;
  t.type("1");
  TEST_ASSERT_EQUAL_UINT16(1, t.seg.shown);
  t.type("2");
  TEST_ASSERT_EQUAL_UINT16(12, t.seg.shown);
  t.type("5");
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-5.1
static void test_fifth_digit_pushes_out_the_oldest() {
  Timer t;
  t.type("12345");
  TEST_ASSERT_EQUAL_UINT16(2345, t.seg.shown);
}

// REQ-3.5, REQ-1.2
static void test_start_runs_countdown_for_the_set_time() {
  Timer t;
  t.type("125");
  t.press();
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(12499);
  TEST_ASSERT_TRUE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(12500, t.clock.relayOffAtMs - t.clock.relayOnAtMs);
}

// REQ-3.6
static void test_second_press_cancels_the_countdown() {
  Timer t;
  t.press();
  t.advance(3000);
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT32(3000, t.clock.relayOffAtMs);
  t.advance(10000);
  TEST_ASSERT_FALSE(t.clock.relay);
}

// Il 7 segmenti mostra il tempo residuo arrotondato al decimo superiore.
static void test_seven_seg_counts_down_during_countdown() {
  Timer t;
  t.type("125");
  t.press();
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
  t.advance(100);
  TEST_ASSERT_EQUAL_UINT16(124, t.seg.shown);
  t.advance(1);
  TEST_ASSERT_EQUAL_UINT16(124, t.seg.shown);
  t.advance(12300);  // residuo 99 ms
  TEST_ASSERT_EQUAL_UINT16(1, t.seg.shown);
  uint32_t writes = t.seg.writes;
  t.advance(98);
  TEST_ASSERT_EQUAL_UINT32(writes, t.seg.writes);  // un invio per decimo, non per ms
}

// REQ-3.7
static void test_seven_seg_returns_to_set_time_after_end() {
  Timer t;
  t.type("125");
  t.press();
  t.advance(12500);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-3.7
static void test_seven_seg_returns_to_set_time_after_cancel() {
  Timer t;
  t.type("125");
  t.press();
  t.advance(5000);
  t.press();
  TEST_ASSERT_EQUAL_UINT16(125, t.seg.shown);
}

// REQ-5.2
static void test_first_digit_after_exposure_starts_a_new_time() {
  Timer t;
  t.type("12");
  t.press();
  t.advance(1200);
  t.type("3");
  TEST_ASSERT_EQUAL_UINT16(3, t.seg.shown);
}

// REQ-5.2: anche dopo un Annullamento.
static void test_first_digit_after_cancel_starts_a_new_time() {
  Timer t;
  t.type("12");
  t.press();
  t.press();
  t.type("3");
  TEST_ASSERT_EQUAL_UINT16(3, t.seg.shown);
}

// REQ-5.9: durante l'Esposizione le cifre non cambiano il tempo impostato.
static void test_digits_are_ignored_during_countdown() {
  Timer t;
  t.type("12");
  t.press();
  t.type("9");
  t.advance(1200);
  TEST_ASSERT_EQUAL_UINT16(12, t.seg.shown);
}

// REQ-3.8 (il beep di errore arriva con il ticket dei suoni)
static void test_start_with_zero_time_does_nothing() {
  Timer t;
  t.type("0");
  t.press();
  TEST_ASSERT_FALSE(t.clock.relay);
  t.advance(1);
  TEST_ASSERT_FALSE(t.clock.relay);
  TEST_ASSERT_EQUAL_UINT16(0, t.seg.shown);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_digits_enter_from_the_right);
  RUN_TEST(test_fifth_digit_pushes_out_the_oldest);
  RUN_TEST(test_start_runs_countdown_for_the_set_time);
  RUN_TEST(test_second_press_cancels_the_countdown);
  RUN_TEST(test_seven_seg_counts_down_during_countdown);
  RUN_TEST(test_seven_seg_returns_to_set_time_after_end);
  RUN_TEST(test_seven_seg_returns_to_set_time_after_cancel);
  RUN_TEST(test_first_digit_after_exposure_starts_a_new_time);
  RUN_TEST(test_first_digit_after_cancel_starts_a_new_time);
  RUN_TEST(test_digits_are_ignored_during_countdown);
  RUN_TEST(test_start_with_zero_time_does_nothing);
  return UNITY_END();
}
